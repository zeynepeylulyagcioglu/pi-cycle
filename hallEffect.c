/*
* hall effect sensor 3144.
*
* http://www.allegromicro.com/~/media/Files/Datasheets/A3141-2-3-4-Datasheet.ashx?la=en
*
*
* looking down from the top of the pyramid = power, ground, output (vout).
*  - vout to gpio2.
*  - enable pullup so don't need a resistor.
*
* http://www.raspberrypi-spy.co.uk/2015/09/how-to-use-a-hall-effect-sensor-with-the-raspberry-pi/
*
* The output of these devices (pin 3) switches low when the magnetic
* field at the Hall element exceeds the operate point threshold (BOP). At
* this point, the output voltage is VOUT(SAT). When the magnetic field
* is reduced to below the release point threshold (BRP), the device
* output goes high. The difference in the magnetic operate and release
* points is called the hysteresis (Bhys) of the device. This built-in
* hysteresis allows clean switching of the output even in the presence
* of external mechanical vibration and electrical noise.
*/

#include "gpio.h"
#include "gpio_extra.h"
#include "uart.h"
#include "printf.h"
#include "timer.h"

void print_magnet(unsigned int val) {
   printf(val ?  "magnet out of range\n" : "magnet detected\n" );
}

void get_speed(void) {
   const gpio_id_t pin = GPIO_PB4;

   const unsigned long wheel_diameter_in = 26;

   const unsigned long ms_per_sec = 1000;
   const unsigned long sec_per_hr = 3600;
   const unsigned long in_per_ft = 12;
   const unsigned long ft_per_mi = 5280;

   // gpio_init();
   // uart_init();

   // // initialize timer
   // timer_init();

   gpio_set_input(pin);
   gpio_set_pullup(pin);

   // pin is 1 when the magnet is out of range of the sensor
   print_magnet(1);

   while(1) {
       unsigned long initial_usecs = timer_get_ticks() / TICKS_PER_USEC; // prev microseconds
       unsigned long initial_msecs = initial_usecs / 1000; // prev milliseconds

       while(gpio_read(pin) == 1) {} // wait for low
       print_magnet(0);
       while(gpio_read(pin) == 0) {} // wait for high
       print_magnet(1);

       unsigned long current_usecs = timer_get_ticks() / TICKS_PER_USEC; // cur microseconds 
       unsigned long current_msecs = current_usecs / 1000; // cur milliseconds

       unsigned long ms_elapsed = current_msecs - initial_msecs; // ms elapsed
       unsigned long seconds_elapsed = ms_elapsed / 1000; // seconds elapsed

       printf("millseconds elapsed: %ld\n", ms_elapsed); // print ms elapsed
       printf("seconds elapsed: %ld\n", seconds_elapsed); // print seconds elapsed
       printf("\n");

       // speed in in / millsecond
       const unsigned long mph_num = wheel_diameter_in * (22) * ms_per_sec * sec_per_hr;
       const unsigned long mph_denom = 7 * ms_elapsed * in_per_ft * ft_per_mi;

       // integer precision
       // const unsigned long mph = mph_num / mph_denom;
       // printf("mph: %ld\n\n\n", mph);

       // 3 decimal precision
       const unsigned long mph_x1000 = 1000 * mph_num / mph_denom;
       const unsigned long mph_1 = mph_x1000 / 1000;
       const unsigned long mph_rest = mph_x1000 % 1000;
       printf("mph_COMPARE: %ld\n", mph_x1000);
       printf("mph: %ld.%03ld\n\n\n", mph_1, mph_rest);
   }
}
