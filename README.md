# Multi-Device Controller
This repository showcases a self-designed framework for controlling multiple programmable integrated circuits through a single Arduino and custom GUI.
An example GUI was created using Python with QT and a custom-made expandable library (PyDuino) which communicates to the Arduino sending a simple,
easily expandable command system. From the Arduino side, the serial commands are recieved through USB, stored in a queue and executed into one main
execution function that can be defined to divert to different backend functions for each individual device to program, to which the Arduino can then
be programmed to send data to the different individual devices through SPI or I2C.

For this project, I will/have implement(ed) the GUI/Arduino to controll 3 separate devices. The first of which is an AD5722/AD5732/AD5752 DAC through SPI,
which was the initial goal of this project. However, to acompany it is futher, it is being expanded to also control an LTC2977 PMIC (controlled by PMBus)
as well as an AD9910 (SPI).

# Execution
The new version of the code is located in /Device_Driver_Main/*
To be uploaded to the Arduino: /Device_Driver_Main/Arduino_Controller/Arduino_Controller.ino

To run the program, run /Device_Driver_Main/gui.py with all dependencies installed (PyQt5 + all libraries in shared folder)

PyQt5 can be installed with pip using the command 'pip install pyqt5'

# DAC-Controller
The initial goal of this program was designed to control an AD5722/AD5732/AD5752 DAC through SPI communication, specifically the AD5732.
This IC is a 14 bit individually addressable dual output DAC. The spec sheet can be seen here: http://www.analog.com/media/en/technical-documentation/data-sheets/AD5722_5732_5752.pdf

## DAC Power-up sequence
1. GND
2. Digital Power (+2.7V to +5.5V)
3. Analog Power (+/- 4.5 to +/- 16.5V) (Positive and Negative squence order do not matter)

## DAC Pinouts
|DAC Pin 	|Pin Description 						|Arduino Pin 	|
|-----------|---------------------------------------|---------------|
|1, EP		|AV<sub>SS</sub>: Negative Analog Power |X				|
|24			|AV<sub>DD</sub>: Positive Analog Power |X				|
|3			|V<sub>out_A</sub>: DAC Output A 		|X				|
|23			|V<sub>out_A</sub>: DAC Output B 		|X				|
|7			|SPI Slave Select						|10				|
|8 			|SPI Clock 								|13				|
|9 			|SPI Slave Data IN (MOSI)				|11				|
|10			|LDAC Select (Active Low)				|8 (or LOW)		|
|16 		|SPI Slave Data OUT (MISO)				|12				|
|11			|CLR (Active Low -> Tied to HIGH)		|HIGH 			|
|15, (18-21)|GND									|GND			|
|14			|DV<sub>CC</sub>: Digital Power 		|+5 (or +3.3?)	|
|17			|Reference Voltage (+2 to +3V)			|X				|
|5			|Offset / 2's Complement				|HIGH			|
|Other pins	|NC - Do Not Connect					|NC 			|

(Digital and Analog Power are both tied to the same GND in this diagram. See spec sheet for specifics.)