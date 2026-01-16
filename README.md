## Project title
Brake Your Bike

## Team members
Zeynep Eylül Yağcıoğlu, Ansh Kharbanda

## Project description
Brake Your Bike is a hardware-software integrated system that enhances bicycle safety by incorporating an automated braking mechanism and turn signaling system. The project combines real-time speed monitoring, accelerometer-based turn detection, and a mechanical braking solution to provide a responsive and efficient biking experience.

The braking system uses a servo motor calibrated through a Pulse Width Modulation (PWM) driver to actuate the bike's brake pads. A Hall Effect sensor is employed to measure the bike's speed, while a graphical display provides the rider with real-time feedback on their speed and braking status.

The turn signaling mechanism leverages the MSA311 I2C accelerometer to detect handlebar movements, distinguishing turns from straight-line cycling. Button-activated interrupts initiate turn monitoring, and LEDs are used to indicate the rider's intention to turn. The system features a custom driver for the accelerometer, designed for flexibility and optimized for continuous data processing.

This project showcases a collaborative effort in integrating mechanical design, electronic systems, and software algorithms to enhance bicycle safety, aiming to provide a comprehensive solution for urban cyclists.

## Member contribution
Together: all testing, all integration, all wiring & cable management, initial brainstorms for stepper motor (Eylül handled mostly everything after)

Ansh: speed calibration using hall effect sensor, display module for speed & braking, ToF sensor calibration (unsuccessful), physical adjustment of bike parts, and donated bike for project

Eylül: I have combined the "Member Contribution" section with the "Challenges & Solutions" and "What did you learn along the way?" portions of the self-reflection part into a single section below. The remaining prompts from the self-reflection part are addressed separately afterward.

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

## Self-evaluation

Ansh: 

(1) How well was your team able to execute on the plan in your proposal?  
Our big lift in this project was getting the bike to brake, which we accomplished successfully. We were also successfully able to measure speed but unsuccessful in measuring distance to nearby objects.
We did add an entirely new component of adding turn signals, which wasn’t in the initial proposal. 

(2) Challenges & their solutions
How do you have cables long enough to manage across the bike? You strip jumper cables, solder stranded wire to both ends, add electrical tape to the soldered connections, and create your own long wires.
How do you mount the hall effect sensor such that it consistently detects the magnet, given the very short range of the sensor on the weak magnet (~1-2 cm). You tape the hall effect sensor on the cardboard box from the servo motor, tape 2 magnets together on the spoke of the bike, and test a lot.
How do you communicate with a device that has all the setup abstracted into an API? Hard to say and we didn’t get this to work, but the starting point is to download the raw C from the API, dig through the required setup functions, look for examples online of simplifications people have made to the setup, and look to always do the least amount of setup you know will be correct before testing.
How do you adjust bike brakes such that they are tight enough so that your braking system can honestly provide enough force to activate it but loose enough that the tire can still turn? You spend a lot of time getting your hands dirty with an allen key and testing.
How do you figure out if the system doesn’t work due to some weird, unpredictable wiring issue or your code? Go through each one of your assumptions one at a time—no matter how basic—and confirm you know the purpose of each wire / component. Doing this enough, testing individual components to isolate the bug, and having patience worked for us even during crunch time. 
How do you integrate code two people wrote and tested separately? Create one simple test file with the core functionality and incorporate components one at a time as you test.

(3) Things we wish we did better
LEDs. Fancier designs to match the biking speed on the outside.
Actually got the ToF sensor to work, or thoroughly checked the datasheet for setup components  before ordering the part. 
I believe that knowing 

(4) What did you learn along the way? Tell us about it!
Mostly in challenges / solutions. The other thing I learned is the importance of trust and patience in a group project, especially in a high-stress environment where everyone is really invested in the entire system working. You have to spend a lot of time figuring things out together, and sometimes make sacrifices to work on other parts of the system to support your partner. This attention management and context switching between the needs of your project, needs of your partner, and needs of the deadline is a non-trivial skill I’m still learning.


Eylül: 

Additional to the reflection I wrote above, I want to mention a few other points.

(1) How well was your team able to execute on the plan in your proposal?  
We were able to realize our initial goal braking system and add on top of to that with the automated turn signaling. I believe that though first part was very fulfilling and enjoyable in terms of designing and solving a feasibility puzzle, one of the motivating sources for the second part was having the opportunity to tackle more out-of-comfort zone challenges in the software realm. I am very glad that we were able to integrate both systems within our demo in conjunction with the graphing display.

(2) Challenges & their solutions
I focused on the main challenges above, but I really want to mention this: SOLDERING and using LASER CUTTING MACHINE! I soldered the pins for both accelerometers, which was a cool challenge and an exciting new skill to learn. Additionally, I completed the training to use the laser cutting machine, designed a rectangle (a hard task :D) and cut it out for mounting the servo motor.

(3) Things we wish we did better
I focused on this above, but to summarize, having the sensor earlier could have improved our time-line but I believe we did a really good job overall. I also believe that becoming more familiar with data sheets and developing an "eye" for making confident judgment calls when selecting sensors could increase our efficiency in the future.

(4) What did you learn along the way? Tell us about it!
I focused on this above, but I would like to highlight this: I believe that trusting each other and finding a balance between challenging our ideas and showing support was crucial to the success of our project. Since we were working on different timelines, we divided our focus to cover different parts of the project. However, during integration, we spent a lot of time together and often had to adjust our priorities to support each other. Balancing the needs of the project, our partner, and the deadline required a lot of diligence and teamwork, and I see this as a really nice opportunity to continue developing such an important soft skill.
Additionally, I came to appreciate once again the value of thinking outside the box when solving problems. There were instances where we needed to address challenges, and sometimes tools from our everyday lives proved to be just as effective as more complex mechanisms or technologies. For example, we used the box of the servo motor to install the magnet for the hall effect sensor and utilized a piece of wood from a laser cutting machine to mount the servo motor.

## Photos
:D