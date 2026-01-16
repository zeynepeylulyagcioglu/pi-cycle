## Project title
Pi-Cycle (a.k.a Brake Your Bike)

## Project description
Brake Your Bike is a hardware-software integrated system that enhances bicycle safety by incorporating an automated braking mechanism and turn signaling system. The project combines real-time speed monitoring, accelerometer-based turn detection, and a mechanical braking solution to provide a responsive and efficient biking experience.

The braking system uses a servo motor calibrated through a Pulse Width Modulation (PWM) driver to actuate the bike's brake pads. A Hall Effect sensor is employed to measure the bike's speed, while a graphical display provides the rider with real-time feedback on their speed and braking status.

The turn signaling mechanism leverages the MSA311 I2C accelerometer to detect handlebar movements, distinguishing turns from straight-line cycling. Button-activated interrupts initiate turn monitoring, and LEDs are used to indicate the rider's intention to turn. The system features a custom driver for the accelerometer, designed for flexibility and optimized for continuous data processing.

This project showcases a collaborative effort in integrating mechanical design, electronic systems, and software algorithms to enhance bicycle safety, aiming to provide a comprehensive solution for urban cyclists.

1) Braking Mechanism:
I made theoretical calculations to determine the necessary net external force and net external torque required, which shaped our final design.
I consulted several people, including Julie Zelenski, Matt Vaska, Jeff Stribling, and Frances Bujah Raphael.
After building the mechanical system, I worked on calibration at both the software and hardware layers:
Software:
I learned about the Pulse Width Modulation (PWM) protocol. The theoretical calculations for the parameters that would correspond to an intended angle of 90 degrees did not match the actual output. This was due to the wires being dense, causing energy dissipation that sometimes resulted in lower angles, flipped directions, or the servo arm locking at the top, preventing it from returning to its original position. Eventually, I found a balance by incrementing the servo by -90 degrees and factoring in energy dissipation to reset its position by +85 degrees.
Hardware:
I worked on finding the fine line between adjusting the wires tightly enough to ensure the servo motor’s arm could achieve the vertical displacement necessary to stop the bike and keeping the wires loose enough that the torque (~27.5 kg*cm) was sufficient to push the arm upwards.

2) Automated Turn Signal Mechanism:
To build the Automated Turn Signal Mechanism, I learned the I2C protocol, wrote a driver for the MSA311 I2C accelerometer, and then designed and programmed a workflow integrating button interrupts, LEDs, and the processing of accelerometer readings.

A. Writing a Driver for the MSA311 I2C Accelerometer:
I aimed to create a driver that wasn’t specific to our implementation but could run the accelerometer under various configurations. To achieve this, I analyzed the datasheet. Rather than focusing on the AdaFruit library, I referred to their example user file to identify all the functions necessary for configuring the accelerometer. This approach helped me define the essential functionalities required to prepare the driver.

Data Processing for Our Implementation:
To handle the calculations, I implemented math functions for absolute values and trigonometric computations. Initially, I used the magnitude of acceleration as part of the calculations, which required implementing a square root function. However, the computational heaviness of this function caused the system to fail in providing continuous acceleration readings. Recognizing this issue, I replaced it with custom math functions leveraging known approximations to achieve better O().

a. Determining the Turn and Eliminating Noise:
When the user presses the button, accelerometer readings are initiated through interrupts, and the LED turns off once a turn is detected. The main challenge was ensuring that horizontal cycling movements did not mistakenly trigger the turn signal.
To address this, I first outlined theoretical expectations for changes in the x-, y-, and z-components of the acceleration. I conducted test runs to observe patterns in the readings and compared them to my theoretical predictions. Based on how I oriented the accelerometers, the x- and z-components were more sensitive to changes. However, both also showed variations during straight-line cycling, which could falsely trigger conditions set on specific intervals.
As a solution, I decided to rely on angle calculations.

b. Angle Calculation:
I transformed the x-, y-, and z-components of the acceleration into polar coordinates to calculate a theta parameter (with the handlebar horizontal, theta was 180 degrees).
This approach succeeded in reducing noise, but the differences between theta values for turns and straight-line cycling were still not distinctive enough. Eventually, I observed that when the handlebar was turned to the side opposite to where the accelerometer was mounted, the accelerometer readings switched signs. This became a clear and distinctive factor. While cycling in a straight line, the accelerometer predominantly remained in one sign, with only occasional opposite signs.

c. Algorithm
This formed the backbone of my algorithm. I kept track of 15 readings and identified it as a turn if more than 10 of them were over +30. Here, there was an AHA! moment: if the user reached the 15th positive reading on the 16th total reading, the logic in the monitor_accelerometer function reset the counters (positive_theta_count and total_readings) after processing the 15th reading. This effectively missed the 16th positive reading because the counters were reset before it could be checked. Consequently, the condition to stop monitoring (positive_theta_count >= 10) was not satisfied when it should have been.
To handle this scenario, I changed the logic to keep track of a sliding window of the last 15 readings rather than resetting the counters. This ensured that all relevant data points were considered without missing critical conditions.

d. Challenge: Multiple Accelerometers:
To integrate multiple accelerometers into the system, I explored three different approaches:
1. Updating the Address:
My initial approach was to reconfigure one of the accelerometers to use an alternate I2C address, as the MSA311 sensors share the same default address. Unfortunately, the specific sensor I used did not support changing the address, rendering this method unfeasible.
2. Using a Multiplexer:
As an alternative, I explored using an I2C multiplexer to manage multiple accelerometers on the same I2C bus. I retrieved the multiplexer from Lab64 late at night (around 2-3 am) and worked on its wiring and configuration. Using the multiplexer’s data sheet, I wrote the necessary code to facilitate communication with the accelerometers through the device. However, there seemed to be an issue with the hardware setup that required further debugging. Due to time constraints, I decided to continue with the 3rd possible solution.
3. Using Two Mango Pis:
The final approach involved splitting the workload across two Mango Pi devices and our computers, with each Pi handling one accelerometer. I installed the accelerometer, LED, and button system on both devices and verified that they worked independently. For demonstration purposes, and to simplify the process (particularly because of GitHub merge conflicts), we decided to run the final demo on one Pi only.
Throughout this process, I found the bus system of the I2C protocol quite fascinating, especially the functionality of the multiplexer as described in its data sheet. Understanding how the multiplexer works has piqued my curiosity, and I am eager to revisit this concept over the break to explore how to successfully implement it in a similar system.

B. Button + LED + Accelerometer Workflow:
I utilized interrupts on the button to trigger LED and accelerometer readings. I believe this was an efficient approach that aligned well with the logic of our system.

## References
Hall Effect Sensor
https://www.youtube.com/shorts/nZJUz3bLPeQ 
Julie’s hall effect starter code

VL53L0x ToF sensor
https://www.st.com/resource/en/datasheet/vl53l0x.pdf 
https://www.st.com/resource/en/user_manual/um2039-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf 
https://www.youtube.com/watch?v=yZGWpDTbjdw (dude is actually Swedish, not Russian sorry)
https://github.com/artfulbytes/vl6180x_vl53l0x_msp430/blob/74077f757891038e91d9229ce346d8c920343264/drivers/vl53l0x.c
Julie’s I2C driver
Help from Julie Zelinski and Ben Ruland

The file “failed_VL53l0X.c” is an adapted implementation of the ToF sensor based on YouTuber “Artful Bytes” c code and Julie’s I2C driver. 

Mechanical Braking System
Lots of help from Matt Vaska, Jeff Stribling, Frances Raphael, and others

SN74HC153 Dual 4-Line to 1-Line Data Selectors/Multiplexers
 Texas Instruments. SN74HC153 Dual 4-Line to 1-Line Data Selectors/Multiplexers Data Sheet. Document No. SCAS024E. Texas Instruments Incorporated, Nov. 2021. PDF.

MSA311 Accelerometer Data Sheet
 MEMSensing Microsystems. MSA311 Ultra-Low Power High-Performance 3-Axis Accelerometer Data Sheet. Version 1.1. MEMSensing Microsystems Co., Ltd., Mar. 2019. PDF.

I2C Driver Code
 Zelenski, Julie. I2C Driver Implementation. Adapted for use with the MSA311 Accelerometer. Original code provided as part of Stanford University CS107E coursework.

PWM Driver Code
 Zelenski, Julie. PWM Driver Implementation. Adapted for use with servo motor control. Original code provided as part of Stanford University CS107E coursework. 

Accelerometer Arduino Demo
MSA311 Accelerometer Demo in Arduino. Example code for configuring and reading data from the MSA311 Accelerometer. Used solely as a reference to verify that the configuration functions implemented in the driver align with the requirements. No code was adopted or derived from this source. 

## Photos
:D
