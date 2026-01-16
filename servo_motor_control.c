/* File: servo_motor_control.c
 * ---------------------------
 * This program demonstrates the control of a servo motor using the PWM module.
 * It initializes the PWM channel and sets specific duty cycles to move the servo 
 * to different angular positions (-90 degrees and +85 degrees).
 *
 * Author: Zeynep Eylül Yağcıoğlu
 */

#include "servo_motor_control.h"

/* Configures the PWM channel for servo control */
void configure_pwm(void) {
    pwm_init();
    pwm_config_channel(PWM_CHANNEL, SERVO_PIN, PWM_FREQUENCY, false);
}

/* Moves the servo to a specific position */
void move_servo(float duty_cycle) {
    pwm_set_duty(PWM_CHANNEL, duty_cycle);
}

/* Adds a delay for the specified time in milliseconds */
void delay_ms(unsigned int ms) {
    timer_delay_ms(ms);
}

int main(void) {
    delay_ms(2000); // Initial delay

    configure_pwm(); // Configure PWM

    move_servo(DUTY_NEG_90); // Move servo to -90 degrees
    delay_ms(10000);         // Hold for 10 seconds

    move_servo(DUTY_POS_85); // Move servo to +85 degrees
    delay_ms(2000);          // Hold for 2 seconds

    return 0;
}
