/* File: accelerometer_button_led.h
 * ------------------------------
 * Header file for accelerometer, button, and LED system.
 *
 * This file provides function declarations, macros, and data structures
 * for interfacing with the MSA311 accelerometer, handling button interrupts,
 * and controlling the LED.
 *
 * Author: Zeynep Eylül Yağcıoğlu
 */

#ifndef ACCELEROMETER_BUTTON_LED_H
#define ACCELEROMETER_BUTTON_LED_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include "gpio.h"
#include "timer.h"

/* MSA311 Accelerometer Macros */
#define MSA311_ADDRESS    0x62
#define EXPECTED_PART_ID  0x13

/* Register Definitions */
#define REG_SOFT_RESET    0x00
#define REG_PART_ID       0x01
#define REG_ACC_X_LSB     0x02
#define REG_ACC_Y_LSB     0x04
#define REG_ACC_Z_LSB     0x06
#define REG_FS_RANGE      0x0F
#define REG_ODR           0x10
#define REG_POWER_MODE    0x11
#define REG_BANDWIDTH     0x12
#define REG_RESOLUTION    0x13

/* Configuration Values */
#define FS_2G             0x00
#define FS_4G             0x01
#define FS_8G             0x02
#define FS_16G            0x03
#define ODR_125HZ         0x07
#define POWER_NORMAL      0x00
#define BANDWIDTH_125HZ   0x07
#define RESOLUTION_14     0x01

/* GPIO Definitions */
#define LED_PIN           GPIO_PB3
#define BUTTON_PIN        GPIO_PB4

/* Accelerometer Device Structure */
typedef struct {
    i2c_device_t *i2c_dev;  // I2C device handle
    int range;              // Current accelerometer range in mg (2000, 4000, etc.)
} msa311_t;

/* Function Prototypes */
/* Accelerometer Functions */
msa311_t *msa311_init(void);
bool msa311_read_raw(msa311_t *msa, int16_t *x_raw, int16_t *y_raw, int16_t *z_raw);
bool msa311_read_acceleration(msa311_t *msa, int *x_mg, int *y_mg, int *z_mg);
void msa311_free(msa311_t *msa);

/* Math Helper Functions */
float calculate_theta(int x_mg, int z_mg);
int custom_abs(int value);
float custom_fabsf(float value);
float custom_atan2(float y, float x);

/* Button and LED Functions */
void config_button(void);
void monitor_accelerometer(msa311_t *msa);
void handle_button_interrupt(void *aux_data);

#endif /* ACCELEROMETER_BUTTON_LED_H */
