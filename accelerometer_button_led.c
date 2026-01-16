/* File: accelerometer_button_led.c
 * ---------------------------------
 * Implementation of the system that integrates the MSA311 accelerometer, a button 
 * with interrupt handling, and an LED for visual feedback. This program initializes 
 * and configures the accelerometer, monitors acceleration data to detect angular 
 * changes, and uses the button to trigger the monitoring process.
 *
 * Features:
 * - MSA311 Accelerometer initialization and configuration.
 * - Button interrupt handling for user input.
 * - LED control for visual feedback during monitoring.
 * - Custom mathematical approximations for angle calculations.
 * - Sliding window algorithm to detect angular changes from accelerometer data.
 *
 * Author: Zeynep Eylül Yağcıoğlu
 */

#include "accelerometer_button_led.h"
#include "uart.h"
#include "printf.h"
#include "malloc.h"
#include "gpio_interrupt.h"
#include "interrupts.h"
#include "pwm.h"

/*********************** ACCELOROMETER SENSOR PART BEGINS *********************************/


/* Initialize MSA311 Accelerometer */
msa311_t *msa311_init(void) {
    gpio_set_function(GPIO_PG13, GPIO_FN_ALT3); // SDA
    gpio_set_function(GPIO_PG12, GPIO_FN_ALT3); // SCL

    i2c_init();

    msa311_t *msa = malloc(sizeof(msa311_t));
    if (!msa) return NULL;

    msa->i2c_dev = i2c_new(MSA311_ADDRESS);
    if (!msa->i2c_dev) {
        free(msa);
        return NULL;
    }

    // Verify Device ID
    uint8_t part_id = i2c_read_reg(msa->i2c_dev, REG_PART_ID);
    if (part_id != EXPECTED_PART_ID) {
        printf("MSA311 ID mismatch! Expected 0x%x, got 0x%x\n", EXPECTED_PART_ID, part_id);
        free(msa);
        return NULL;
    }

    // Soft reset
    i2c_write_reg(msa->i2c_dev, REG_SOFT_RESET, 0x01);
    timer_delay_us(1000);

    // Apply default configurations
    set_range(msa, FS_4G);
    set_data_rate(msa, ODR_125HZ);
    set_power_mode(msa, POWER_NORMAL);
    set_bandwidth(msa, BANDWIDTH_125HZ);
    set_resolution(msa, RESOLUTION_14);

    return msa;
}


/* Configuration Functions */
static bool set_range(msa311_t *msa, uint8_t range) {
    assert(range == FS_2G || range == FS_4G || range == FS_8G || range == FS_16G);

    // Write range value to register
    if (!i2c_write_reg(msa->i2c_dev, REG_FS_RANGE, range)) {
        assert(false && "Failed to set range: I2C write error");
        return false;
    }

    // Map range value to corresponding milli-g range
    msa->range = (range == FS_2G) ? 2000 :
                 (range == FS_4G) ? 4000 :
                 (range == FS_8G) ? 8000 : 16000;

    // Add delay for sensor configuration
    timer_delay_us(100);
    return true;
}

static bool set_data_rate(msa311_t *msa, uint8_t data_rate) {
    assert(data_rate <= 0x0F); // Valid data rate values are 0x00 to 0x0F

    if (!i2c_write_reg(msa->i2c_dev, REG_ODR, data_rate)) {
        assert(false && "Failed to set data rate: I2C write error");
        return false;
    }

    // Add delay for sensor configuration
    timer_delay_us(100);
    return true;
}

static bool set_power_mode(msa311_t *msa, uint8_t power_mode) {
    assert(power_mode == POWER_NORMAL); 

    if (!i2c_write_reg(msa->i2c_dev, REG_POWER_MODE, power_mode)) {
        assert(false && "Failed to set power mode: I2C write error");
        return false;
    }

    // Add delay for sensor configuration
    timer_delay_us(100);
    return true;
}

static bool set_bandwidth(msa311_t *msa, uint8_t bandwidth) {
    assert(bandwidth <= 0x0F); // Valid bandwidth values are 0x00 to 0x0F

    if (!i2c_write_reg(msa->i2c_dev, REG_BANDWIDTH, bandwidth)) {
        assert(false && "Failed to set bandwidth: I2C write error");
        return false;
    }

    // Add delay for sensor configuration
    timer_delay_us(100);
    return true;
}

static bool set_resolution(msa311_t *msa, uint8_t resolution) {
    assert(resolution == RESOLUTION_14); 

    if (!i2c_write_reg(msa->i2c_dev, REG_RESOLUTION, resolution)) {
        assert(false && "Failed to set resolution: I2C write error");
        return false;
    }

    // Add delay for sensor configuration
    timer_delay_us(100);
    return true;
}

bool msa311_read_raw(msa311_t *msa, int16_t *x_raw, int16_t *y_raw, int16_t *z_raw) {
    uint8_t data[6];

    // Read 6 bytes from the accelerometer starting from the X-axis LSB register
    if (!i2c_read_reg_n(msa->i2c_dev, REG_ACC_X_LSB, data, 6)) {
        printf("Error: Failed to read raw accelerometer data.\n");
        return false;
    }

    // Combine MSB and LSB for each axis (12-bit data)
    *x_raw = ((data[1] << 4) | (data[0] & 0x0F));
    *y_raw = ((data[3] << 4) | (data[2] & 0x0F));
    *z_raw = ((data[5] << 4) | (data[4] & 0x0F));

    // Perform 12-bit sign extension for negative values
    if (*x_raw & 0x0800) *x_raw |= 0xF000; // Sign-extend bit 11
    if (*y_raw & 0x0800) *y_raw |= 0xF000;
    if (*z_raw & 0x0800) *z_raw |= 0xF000;

    return true;
}

bool msa311_read_acceleration(msa311_t *msa, int *x_mg, int *y_mg, int *z_mg) {
    int16_t x_raw, y_raw, z_raw;

    // Read raw accelerometer data
    if (!msa311_read_raw(msa, &x_raw, &y_raw, &z_raw)) {
        printf("Error: Failed to read acceleration data.\n");
        return false;
    }

    // Convert raw data to milli-g values
    *x_mg = (x_raw * msa->range) / 1024;
    *y_mg = (y_raw * msa->range) / 1024;
    *z_mg = (z_raw * msa->range) / 1024;

    return true;
}

/*
float simple_sqrtf(float number) {
    if (number < 0) {
        return -1; // Error: negative input
    }

    float x = number;
    float y = 1.0f;
    float epsilon = 0.00001f; // Convergence threshold

    while (x - y > epsilon) {
        x = (x + y) / 2;
        y = number / x;
    }
    return x;
}

int msa311_calculate_magnitude(int x_mg, int y_mg, int z_mg) {
    float magnitude = simple_sqrtf((x_mg * x_mg) + (y_mg * y_mg) + (z_mg * z_mg)) / 1000.0;
    // Convert magnitude to a scaled integer for two decimal places
    int magnitude_scaled = (int)(magnitude * 100);
    return magnitude_scaled;
}
*/

int custom_abs(int value) {
    // Returns the absolute value of an integer
    return (value < 0) ? -value : value;
}

float custom_fabsf(float value) {
    // Returns the absolute value of a float
    return (value < 0.0f) ? -value : value;
}

float custom_atan2(float y, float x) {
    // Custom atan2 implementation to handle edge cases
    if (x == 0.0f) {
        if (y == 0.0f) {
            return 0.0f; // Both x and y are zero, theta is undefined; return 0 as a fallback
        }
        return (y > 0.0f) ? 1.5708f : -1.5708f; // ±π/2 for vertical line
    }

    float atan;
    float z = y / x;

    if (custom_fabsf(z) < 1.0f) {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f) {
            return (y < 0.0f) ? atan - 3.14159f : atan + 3.14159f; // Adjust for ±π
        }
    } else {
        atan = 1.5708f - z / (z * z + 0.28f); // π/2 - approximation
        if (y < 0.0f) {
            return atan - 3.14159f; // Adjust for 3rd/4th quadrant
        }
    }
    return atan;
}

float calculate_theta(int x_mg, int z_mg) {
    // Handle cases where both x and z are zero
    if (x_mg == 0 && z_mg == 0) {
        printf("Warning: Both X and Z readings are zero. Theta is undefined.\n");
        return 0.0f; // Default to 0 degrees
    }

    // Calculate theta in radians using custom atan2
    float theta_rad = custom_atan2((float)z_mg, (float)x_mg);

    // Convert to degrees
    float theta_deg = theta_rad * (180.0f / 3.14159f);
    return theta_deg;
}

/* Free MSA311 Resources */
void msa311_free(msa311_t *msa) {
    if (!msa) return;
    i2c_free(msa->i2c_dev);
    free(msa);
}

/*********************** ACCELOROMETER SENSOR PART ENDS *********************************/



/*********************** BUTTON & LED PART BEGINS *********************************/

/* GPIO pin for LED and Button */
#define LED_PIN GPIO_PB3
#define BUTTON_PIN GPIO_PB4

/* Volatile flags for interrupt synchronization */
static volatile bool button_pressed = false;
static volatile bool reading_accel = false;

/* Interrupt handler for button press */
void handle_button_interrupt(void *aux_data) {
    button_pressed = true; // Set the button pressed flag
    gpio_interrupt_clear(BUTTON_PIN); // Clear the interrupt
}

/* Function to monitor accelerometer readings */
void monitor_accelerometer(msa311_t *msa) {
    int x_mg, y_mg, z_mg;
    int theta_window[15] = {0}; // Circular buffer to store the last 15 readings
    int window_start = 0;
    int positive_theta_count = 0;

    while (reading_accel) {
        if (msa311_read_acceleration(msa, &x_mg, &y_mg, &z_mg)) {
            float theta = calculate_theta(x_mg, z_mg);
            int theta_int = (int)(theta * 100); // Scale to two decimal places

            printf("Theta: %d.%02d degrees | Accel (mg) -> X: %d, Y: %d, Z: %d\n", 
                   theta_int / 100, custom_abs(theta_int % 100), x_mg, y_mg, z_mg);

            // Update the sliding window
            if (theta > 30) {
                positive_theta_count += 1 - theta_window[window_start];
                theta_window[window_start] = 1; // Current reading is positive
            } else {
                positive_theta_count -= theta_window[window_start];
                theta_window[window_start] = 0; // Current reading is not positive
            }

            // Move window start to the next position
            window_start = (window_start + 1) % 15;

            // Check if we meet the condition
            if (positive_theta_count >= 10) {
                gpio_write(LED_PIN, 0); // Turn off LED
                reading_accel = false; // Stop monitoring
                break;
            }
        } else {
            printf("Failed to read accelerometer data\n");
        }

        timer_delay_us(100000); // 100ms delay
    }
}


/*********************** BUTTON & LED PART ENDS *********************************/


void main(void) {
    gpio_init();
    interrupts_init();

    // Initialize LED pin
    gpio_set_output(LED_PIN);
    gpio_write(LED_PIN, 0); // Turn off LED initially

    // Configure button with interrupt
    config_button();

    // Initialize accelerometer
    msa311_t *msa = msa311_init();
    if (!msa) {
        printf("Failed to initialize accelerometer!\n");
        return;
    }

    // Enable global interrupts
    interrupts_global_enable();

    printf("System initialized. Waiting for button press...\n");

    while (true) {
        if (button_pressed) {
            button_pressed = false; // Reset flag

            if (!reading_accel) {
                gpio_write(LED_PIN, 1); // Turn on LED
                reading_accel = true;
                printf("Button pressed. Monitoring accelerometer...\n");
                monitor_accelerometer(msa);
            }
        }

        // Small delay to reduce CPU usage
        timer_delay_us(10000);
    }

    msa311_free(msa);
}


