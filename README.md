# DAC-Controller
Simple Python/QT/Arduino program designed to control an AD5722/AD5732/AD5752 DAC through SPI communication, specifically the AD5732. This IC is a 14 bit individually addressable dual output DAC. The spec sheet can be seen here: http://www.analog.com/media/en/technical-documentation/data-sheets/AD5722_5732_5752.pdf

# Execution
The new version of the code is located in /DAC_Driver_GUI/*
To be uploaded to the Arduino: /DAC_Driver_GUI/DAC_Driver/Dac_Driver.ino

To run the program, run /DAC_Driver_GUI/gui.py with all dependencies installed (PyQt5 + all libraries in shared folder)

PyQt5 can be installed with pip using the command 'pip install pyqt5'

# DAC Power-up sequence
1. GND
2. Digital Power (+2.7V to +5.5V)
3. Analog Power (+/- 4.5 to +/- 16.5V) (Positive and Negative squence order do not matter)

# Pinouts
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
|The Rest	|NC 									|NC 			|
(Digital and Analog Power are both tied to the same GND in this diagram. See spec sheet for specifics.)