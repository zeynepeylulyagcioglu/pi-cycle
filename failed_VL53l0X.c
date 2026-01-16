/*
   VL53L0X device setup.
   Lots of code understood and taken from https://github.com/artfulbytes/vl6180x_vl53l0x_msp430/blob/74077f757891038e91d9229ce346d8c920343264/drivers/vl53l0x.c
*/

#include "uart.h"
#include "mymodule.h"
#include "i2c.h"
#include "timer.h"
#include "printf.h"
#include "assert.h"

#define VL53L0X_ADDR 0x29
#define REG_IDENTIFICATION_MODEL_ID 0xC0
#define VL53L0X_EXPECTED_DEVICE_ID 0xEE
#define REG_RESULT_RANGE_STATUS 0x14
#define REG_SYSRANGE_START 0x00
#define REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV 0x89
#define REG_SYSTEM_INTERRUPT_CONFIG_GPIO 0x0A
#define REG_GPIO_HV_MUX_ACTIVE_HIGH 0x84
#define REG_SYSTEM_INTERRUPT_CLEAR 0x0B
#define REG_SYSTEM_SEQUENCE_CONFIG 0x01
#define RANGE_SEQUENCE_STEP_DSS 0x28 // Dynamic SPAD selection
#define RANGE_SEQUENCE_STEP_PRE_RANGE 0x40
#define RANGE_SEQUENCE_STEP_FINAL_RANGE 0x80
#define REG_RESULT_INTERRUPT_STATUS 0x13

typedef enum {
   CALIBRATION_TYPE_VHV,
   CALIBRATION_TYPE_PHASE
} calibration_type_t;

static i2c_device_t *vl50l0x_init(void) {
   i2c_device_t *dev = i2c_new(VL53L0X_ADDR);
   assert(dev != NULL);
   assert(i2c_read_reg(dev, REG_IDENTIFICATION_MODEL_ID) == VL53L0X_EXPECTED_DEVICE_ID);

   return dev;
}

static bool data_init(i2c_device_t *dev) {
   bool success = false;

   // Set 2v8 mode
   uint8_t vhv_config_scl_sda = 0;
   vhv_config_scl_sda |= 0x01;
   if (!i2c_write_reg(dev, REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, vhv_config_scl_sda)) {
       return false;
   }

   // Set I2C standard mode
   success = i2c_write_reg(dev, 0x88, 0x00);
   success &= i2c_write_reg(dev, 0x80, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x00, 0x00);
   success &= i2c_write_reg(dev, 0x00, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x88, 0x00);

   return success;
}

static bool load_default_tuning_settings(i2c_device_t *dev) {
   bool success = i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x00, 0x00);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x09, 0x00);
   success &= i2c_write_reg(dev, 0x10, 0x00);
   success &= i2c_write_reg(dev, 0x11, 0x00);
   success &= i2c_write_reg(dev, 0x24, 0x01);
   success &= i2c_write_reg(dev, 0x25, 0xFF);
   success &= i2c_write_reg(dev, 0x75, 0x00);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x4E, 0x2C);
   success &= i2c_write_reg(dev, 0x48, 0x00);
   success &= i2c_write_reg(dev, 0x30, 0x20);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x30, 0x09);
   success &= i2c_write_reg(dev, 0x54, 0x00);
   success &= i2c_write_reg(dev, 0x31, 0x04);
   success &= i2c_write_reg(dev, 0x32, 0x03);
   success &= i2c_write_reg(dev, 0x40, 0x83);
   success &= i2c_write_reg(dev, 0x46, 0x25);
   success &= i2c_write_reg(dev, 0x60, 0x00);
   success &= i2c_write_reg(dev, 0x27, 0x00);
   success &= i2c_write_reg(dev, 0x50, 0x06);
   success &= i2c_write_reg(dev, 0x51, 0x00);
   success &= i2c_write_reg(dev, 0x52, 0x96);
   success &= i2c_write_reg(dev, 0x56, 0x08);
   success &= i2c_write_reg(dev, 0x57, 0x30);
   success &= i2c_write_reg(dev, 0x61, 0x00);
   success &= i2c_write_reg(dev, 0x62, 0x00);
   success &= i2c_write_reg(dev, 0x64, 0x00);
   success &= i2c_write_reg(dev, 0x65, 0x00);
   success &= i2c_write_reg(dev, 0x66, 0xA0);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x22, 0x32);
   success &= i2c_write_reg(dev, 0x47, 0x14);
   success &= i2c_write_reg(dev, 0x49, 0xFF);
   success &= i2c_write_reg(dev, 0x4A, 0x00);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x7A, 0x0A);
   success &= i2c_write_reg(dev, 0x7B, 0x00);
   success &= i2c_write_reg(dev, 0x78, 0x21);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x23, 0x34);
   success &= i2c_write_reg(dev, 0x42, 0x00);
   success &= i2c_write_reg(dev, 0x44, 0xFF);
   success &= i2c_write_reg(dev, 0x45, 0x26);
   success &= i2c_write_reg(dev, 0x46, 0x05);
   success &= i2c_write_reg(dev, 0x40, 0x40);
   success &= i2c_write_reg(dev, 0x0E, 0x06);
   success &= i2c_write_reg(dev, 0x20, 0x1A);
   success &= i2c_write_reg(dev, 0x43, 0x40);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x34, 0x03);
   success &= i2c_write_reg(dev, 0x35, 0x44);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x31, 0x04);
   success &= i2c_write_reg(dev, 0x4B, 0x09);
   success &= i2c_write_reg(dev, 0x4C, 0x05);
   success &= i2c_write_reg(dev, 0x4D, 0x04);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x44, 0x00);
   success &= i2c_write_reg(dev, 0x45, 0x20);
   success &= i2c_write_reg(dev, 0x47, 0x08);
   success &= i2c_write_reg(dev, 0x48, 0x28);
   success &= i2c_write_reg(dev, 0x67, 0x00);
   success &= i2c_write_reg(dev, 0x70, 0x04);
   success &= i2c_write_reg(dev, 0x71, 0x01);
   success &= i2c_write_reg(dev, 0x72, 0xFE);
   success &= i2c_write_reg(dev, 0x76, 0x00);
   success &= i2c_write_reg(dev, 0x77, 0x00);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x0D, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x80, 0x01);
   success &= i2c_write_reg(dev, 0x01, 0xF8);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x8E, 0x01);
   success &= i2c_write_reg(dev, 0x00, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x80, 0x00);

   return success;
}

static bool configure_interrupt(i2c_device_t *dev) {
   // Interrupt on new sample ready
   if (!i2c_write_reg(dev, REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04)) {
       return false;
   }

   // Configure active low since the pin is pulled-up on most breakout boards
   uint8_t gpio_hv_mux_active_high = 0;
   gpio_hv_mux_active_high &= ~0x10;

   if (!i2c_write_reg(dev, REG_GPIO_HV_MUX_ACTIVE_HIGH, gpio_hv_mux_active_high)) {
       return false;
   }

   if (!i2c_write_reg(dev, REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
       return false;
   }

   return true;
}

static bool set_sequence_steps_enabled(i2c_device_t *dev, uint8_t sequence_step) {
   return i2c_write_reg(dev, REG_SYSTEM_SEQUENCE_CONFIG, sequence_step);
}

static bool static_init(i2c_device_t *dev) {
   // ISSUE WITH DEFAULT TUNING SETTINGS - LEAVE OUT FOR NOW
   if (!load_default_tuning_settings(dev)) {
       printf("LOAD FAILED...\n");
       return false;
   }

   if (!configure_interrupt(dev)) {
       printf("CONFIG INTERRUPT FAILED...\n");
       return false;
   }

   if (!set_sequence_steps_enabled(dev, (RANGE_SEQUENCE_STEP_DSS +
                                       RANGE_SEQUENCE_STEP_PRE_RANGE +
                                       RANGE_SEQUENCE_STEP_FINAL_RANGE))) {
       printf("SEQUENCE FAILED...\n");
       return false;
   }

   return true;
}

static bool perform_single_ref_calibration(i2c_device_t *dev, calibration_type_t calib_type) {
   uint8_t sysrange_start = 0;
   uint8_t sequence_config = 0;

   switch (calib_type) {
       case CALIBRATION_TYPE_VHV:
           sequence_config = 0x01;
           sysrange_start = 0x01 | 0x40;
           break;
       case CALIBRATION_TYPE_PHASE:
           sequence_config = 0x02;
           sysrange_start = 0x01 | 0x00;
           break;
   }

   if (!i2c_write_reg(dev, REG_SYSTEM_SEQUENCE_CONFIG, sequence_config)) {
       printf("CONFIG FAILED...\n");
       return false;
   }

   if (!i2c_write_reg(dev, REG_SYSRANGE_START, sysrange_start)) {
       printf("START FAILED...\n");
       return false;
   }

   // Wait for interrupt
   uint8_t interrupt_status = 0;
   do {
       interrupt_status = i2c_read_reg(dev, REG_RESULT_INTERRUPT_STATUS);
   } while ((interrupt_status & 0x07) == 0);

   // If no relevant interrupt is detected, return false
   if ((interrupt_status & 0x07) == 0) {
       printf("SUCCESS FAILED...\n");
       return false;
   }

   if (!i2c_write_reg(dev, REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
       printf("INTERRUPT FAILED...\n");
       return false;
   }

   if (!i2c_write_reg(dev, REG_SYSRANGE_START, 0x00)) {
       printf("START SYSRANGE FAILED...\n");
       return false;
   }

   return true;
}

/*
* Temperature calibration needs to be run again if the temperature changes by
* more than 8 degrees according to the datasheet.
*/

static bool perform_ref_calibration(i2c_device_t *dev) {
   if (!perform_single_ref_calibration(dev, CALIBRATION_TYPE_VHV)) {
       printf("VHV FAILED...\n");
       return false;
   }

   if (!perform_single_ref_calibration(dev, CALIBRATION_TYPE_PHASE)) {
       printf("PHASE FAILED...\n");
       return false;
   }

   /* Restore sequence steps enabled */
   if (!set_sequence_steps_enabled(dev, (RANGE_SEQUENCE_STEP_DSS +
                                   RANGE_SEQUENCE_STEP_PRE_RANGE +
                                   RANGE_SEQUENCE_STEP_FINAL_RANGE))) {
       printf("SEQUENCE FAILED...\n");
       return false;
   }

   return true;
}

bool vl53l0x_read_range_single(i2c_device_t *dev, uint16_t *range) {
   bool success = i2c_write_reg(dev, 0x80, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x01);
   success &= i2c_write_reg(dev, 0x00, 0x00);

   // not sure what this does
   // success &= i2c_write_reg(dev, 0x91, stop_variable);

   success &= i2c_write_reg(dev, 0x00, 0x01);
   success &= i2c_write_reg(dev, 0xFF, 0x00);
   success &= i2c_write_reg(dev, 0x80, 0x00);

   if (!success) {
       return false;
   }

   if (!i2c_write_reg(dev, REG_SYSRANGE_START, 0x01)) {
       return false;
   }

   uint8_t sysrange_start;
   do {
       sysrange_start = i2c_read_reg(dev, REG_SYSRANGE_START);
   } while (sysrange_start & 0x01);

   uint8_t interrupt_status;
   do {
       interrupt_status = i2c_read_reg(dev, REG_RESULT_INTERRUPT_STATUS);
   } while ((interrupt_status & 0x07) == 0);

   if (!success) {
       return false;
   }

   uint8_t high_byte = i2c_read_reg(dev, REG_RESULT_RANGE_STATUS + 10);
   uint8_t low_byte = i2c_read_reg(dev, REG_RESULT_RANGE_STATUS + 11);

   uint16_t range_comp_val = ((uint16_t)high_byte << 8) | low_byte;

   if (*range != range_comp_val) {
       return false;
   }

   // if (!i2c_write_reg(dev, REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
   //     return false;
   // }

   return true;
}

void main(void) {
   uart_init();
   i2c_init();
   timer_delay_ms(100); // wait for sensor to leave standby

   i2c_device_t *tof_sensor = vl50l0x_init();

   if (!data_init(tof_sensor)) {
       printf("DATA INIT FAILED...\n");
       return;
   }

   // if (!static_init(tof_sensor)) {
   //     printf("STATIC INIT FAILED...\n");
   //     return;
   // }

   // if (!perform_ref_calibration(tof_sensor)) {
   //     printf("REF CALIBRATION FAILED...\n");
   //     return;
   // }

   printf("VL53L0X initialized successfully!\n");

   uint16_t range = 0;

   if (!vl53l0x_read_range_single(tof_sensor, &range)) {
       printf("Range reading failed\n");
       return;
   }

   // while (1) {
   //     // Get range reading
   //     uint16_t range_mm = vl53l0x_get_range(tof_sensor);
   //     printf("Distance: %d mm\n", range_mm);

   //     // Delay before the next measurement
   //     timer_delay_ms(100);
   // }
}

// static uint16_t vl53l0x_get_range(i2c_device_t *dev) {
//     // Start a range measurement
//     i2c_write_reg(dev, REG_SYSRANGE_START, 0x01);

//     // Wait for measurement to complete (default ~30ms)
//     timer_delay_ms(30);

//     // Read range result
//     uint8_t range = i2c_read_reg(dev, REG_RESULT_RANGE_STATUS);
//     return range; // Range in mm
// }

