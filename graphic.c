/*
    VL53L0X device setup.
    Lots of code understood and taken from https://github.com/artfulbytes/vl6180x_vl53l0x_msp430/blob/74077f757891038e91d9229ce346d8c920343264/drivers/vl53l0x.c
*/

#include "uart.h"
#include "mymodule.h"
#include "timer.h"
#include "printf.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "assert.h"
#include "fb.h"
#include "gl.h"

/* LIBRARIES FOR MOTOR BEGIN */
#include "pwm.h"
/* LIBRARIES FOR MOTOR END */

#define WHEEL_DIAMETER_IN 26
#define MS_PER_SEC 1000
#define SEC_PER_HR 3600
#define IN_PER_FT 12
#define FT_PER_KM 3281

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

            // BRAKING MECHANISM BEGINS
            timer_delay_ms(5000);
            
            // BRAKING MECHANISM ENDS


            // resting screen
            gl_clear(gl_color(102, 255, 255)); // light blue
            gl_draw_string(10, 35, "PHEW!", GL_BLACK);
            gl_draw_string(10, 75, "close call...", GL_BLACK); // fix alignment
            gl_swap_buffer();

            nextStage = true;
        }
	}

}