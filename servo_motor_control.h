/* File: servo_motor_control.h
 * ---------------------------
 * Header file for controlling a servo motor using PWM.
 *
 * This file defines the macros and function declarations used for
 * initializing and configuring the PWM module, and moving the servo motor.
 *
 * Author: Zeynep Eylül Yağcıoğlu
 */

#ifndef SERVO_MOTOR_CONTROL_H
#define SERVO_MOTOR_CONTROL_H

#include "pwm.h"
#include "gpio.h"
#include "timer.h"

/* Macros */
#define PWM_CHANNEL      PWM4       // PWM channel for the servo motor
#define SERVO_PIN        GPIO_PB1   // GPIO pin connected to the servo
#define PWM_FREQUENCY    50         // Frequency in Hz (typical for servos)
#define DUTY_NEG_90      6.5        // Duty cycle for -90 degrees
#define DUTY_POS_85      9.5        // Duty cycle for +85 degrees

/* Function Declarations */
void configure_pwm(void);
void move_servo(float duty_cycle);
void delay_ms(unsigned int ms);

#endif /* SERVO_MOTOR_CONTROL_H */
