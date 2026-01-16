/* File: bike_demo.c
 * -----------------
 * This file implements the complete system integration for the "Brake Your Bike" project. 
 * It demonstrates the functionality of our smart bike system by integrating various hardware 
 * components, including a Hall effect sensor, accelerometer, LED, servo motor, and a graphical 
 * display module. The project emphasizes user safety by providing real-time speed monitoring, 
 * automated braking, and turn signaling mechanisms.
 *
 * Project Title:
 * Brake Your Bike
 *
 * Features:
 * - Speed Detection: Utilizes a Hall effect sensor to calculate and display the bike's speed.
 * - *Graphical Interface: A graphical display with dynamic feedback based on speed thresholds:
 *      - Green: Safe speed range.
 *      - Yellow: Warning range (slowing recommended).
 *      - Red: Braking triggered due to overspeeding.
 * - Braking System: A servo motor controls the mechanical braking mechanism when the speed 
 *   exceeds a safe threshold.
 * - Turn Signaling: Incorporates accelerometer-based monitoring to detect angular turns and 
 *   provide automated turn signals using LEDs.
 * - User Interaction: A button enables user-triggered accelerometer monitoring.
 *
 *
 * Authors: Zeynep Eylül Yağcıoğlu, Ansh Kharbanda
 */

#include "uart.h"
#include "timer.h"
#include "printf.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "assert.h"
#include "fb.h"
#include "gl.h"
#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>
#include "malloc.h"
#include "gpio_interrupt.h"
#include "gl.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "pwm.h"

#define WHEEL_DIAMETER_IN 26
#define MS_PER_SEC 1000
#define SEC_PER_HR 3600
#define IN_PER_FT 12
#define FT_PER_KM 3281


/*********************** ACCELOROMETER SENSOR PART BEGINS *********************************/


/* MSA311 I2C Address */
#define MSA311_ADDRESS 0x62

/* Register Definitions */
#define REG_SOFT_RESET  0x00
#define REG_PART_ID     0x01
#define REG_ACC_X_LSB   0x02
#define REG_ACC_Y_LSB   0x04
#define REG_ACC_Z_LSB   0x06
#define REG_FS_RANGE    0x0F
#define REG_ODR         0x10
#define REG_POWER_MODE  0x11
#define REG_BANDWIDTH   0x12
#define REG_RESOLUTION  0x13

/* Configuration Values */
#define FS_2G           0x00
#define FS_4G           0x01
#define FS_8G           0x02
#define FS_16G          0x03

#define ODR_125HZ       0x07
#define POWER_NORMAL    0x00
#define BANDWIDTH_125HZ 0x07
#define RESOLUTION_14   0x01

/* Expected Device ID for MSA311 */
#define EXPECTED_PART_ID 0x13

/* Accelerometer Device Structure */
typedef struct {
    i2c_device_t *i2c_dev;  // I2C device handle
    int range;              // Current accelerometer range in mg (2000, 4000, etc.)
} msa311_t;

/* Function Prototypes */
static bool set_range(msa311_t *msa, uint8_t range);
static bool set_data_rate(msa311_t *msa, uint8_t data_rate);
static bool set_power_mode(msa311_t *msa, uint8_t power_mode);
static bool set_bandwidth(msa311_t *msa, uint8_t bandwidth);
static bool set_resolution(msa311_t *msa, uint8_t resolution);

bool msa311_read_raw(msa311_t *msa, int16_t *x_raw, int16_t *y_raw, int16_t *z_raw);
bool msa311_read_acceleration(msa311_t *msa, int *x_mg, int *y_mg, int *z_mg);
int msa311_calculate_magnitude(int x_mg, int y_mg, int z_mg);
void msa311_free(msa311_t *msa);

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

/* Function to configure the button with interrupt */
void config_button(void) {
    gpio_set_input(BUTTON_PIN);           // Set button as input
    gpio_set_pullup(BUTTON_PIN);          // Enable internal pull-up resistor
    gpio_interrupt_init();                // Initialize GPIO interrupt system
    gpio_interrupt_config(BUTTON_PIN, GPIO_INTERRUPT_NEGATIVE_EDGE, true); // Trigger on falling edge
    gpio_interrupt_register_handler(BUTTON_PIN, handle_button_interrupt, NULL); // Register interrupt handler
    gpio_interrupt_enable(BUTTON_PIN);   // Enable interrupt for button pin
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


void main2(void) {
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

void print_magnet(unsigned int val) {
    printf(val ?  "magnet out of range\n" : "magnet detected\n" );
}

void main(void) {

    // initialize modules
    gpio_init();
    uart_init();
    timer_init();
    pwm_init();

 
    // set GPIO pin for hall effect
    const gpio_id_t hall_effect = GPIO_PC1;
    gpio_set_input(hall_effect);
  	gpio_set_pullup(hall_effect);

    // pin is 1 when the magnet is out of range of the sensor
    print_magnet(1);

    // initialize display
    const int WIDTH = 200;
    const int HEIGHT = 140;
    gl_init(WIDTH, HEIGHT, GL_DOUBLEBUFFER);
    gl_clear(gl_color(0, 179, 89)); // light blue
    gl_draw_string(10, 35, "speed: ...", GL_BLACK);

    // display
    gl_swap_buffer();

    bool nextStage = false;
	while(!nextStage) {
		unsigned long initial_usecs = timer_get_ticks() / TICKS_PER_USEC; // prev microseconds
		unsigned long initial_msecs = initial_usecs / 1000; // prev milliseconds

  		while(gpio_read(hall_effect) == 1) {} // wait for low
		print_magnet(0);
  		while(gpio_read(hall_effect) == 0) {} // wait for high
		print_magnet(1);

		unsigned long current_usecs = timer_get_ticks() / TICKS_PER_USEC; // cur microseconds 
		unsigned long current_msecs = current_usecs / 1000; // cur milliseconds

		unsigned long ms_elapsed = current_msecs - initial_msecs; // ms elapsed
		unsigned long seconds_elapsed = ms_elapsed / 1000; // seconds elapsed

		// integer precision
		const unsigned long kph_num = WHEEL_DIAMETER_IN * (22) * MS_PER_SEC * SEC_PER_HR;
		const unsigned long kph_denom = 7 * ms_elapsed * IN_PER_FT * FT_PER_KM;
		const unsigned long kph = kph_num / kph_denom;
		printf("kph: %ld\n\n\n", kph);

		// 3 decimal precision
		// const unsigned long kph_x1000 = 1000 * kph_num / kph_denom;
		// const unsigned long kph_1 = kph_x1000 / 1000;
		// const unsigned long kph_rest = kph_x1000 % 1000;
		// printf("kph: %ld.%03ld\n\n\n", kph_1, kph_rest);

        size_t bufsize = 20;
        char speed_buffer[bufsize];
        snprintf(speed_buffer, bufsize, "speed: %ld kph", kph);

        // change display based on speed
        if (kph >= 0 && kph <= 8) {
            gl_clear(gl_color(0, 179, 89)); // green
            gl_draw_string(10, 35, speed_buffer, GL_BLACK);
            gl_swap_buffer();
        } else if (kph >= 9 && kph <= 12) {
            gl_clear(gl_color(255, 255, 0)); // yellow
            gl_draw_string(10, 35, speed_buffer, GL_BLACK);
            gl_draw_string(30, 75, "SLOW DOWN!", GL_BLACK);
            gl_swap_buffer();
        } else if (kph >= 13) {
            gl_clear(gl_color(255, 51, 0)); // red
            gl_draw_string(10, 35, speed_buffer, GL_BLACK);
            gl_draw_string(35, 75, "BRAKING!", GL_BLACK); // fix aligment
            gl_swap_buffer();

            // DELAY IN PLACE OF BRAKING MECHANISM
            timer_delay_ms(5000);

            // Configure PWM channel 4 (PB1) with a 50 Hz frequency (typical for servos)
            pwm_config_channel(PWM4, GPIO_PB1, 50, false);

            // Move servo to -90 degrees
            pwm_set_duty(PWM4, 6.5); // Duty cycle for approx. -90 degrees
            timer_delay_ms(10000);     // Hold for 2 seconds

            // Move servo to +85 degrees
            pwm_set_duty(PWM4, 9.5); // Duty cycle for approx. +85 degrees
            timer_delay_ms(2000);     // Hold for 2 seconds

            // resting screen
            gl_clear(gl_color(102, 255, 255)); // light blue
            gl_draw_string(10, 35, "PHEW!", GL_BLACK);
            gl_draw_string(10, 75, "close call...", GL_BLACK); // fix alignment
            gl_swap_buffer();

            nextStage = true;
        }
	}
    main2();
}