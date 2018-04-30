#####################################
# Arduino Serial Communication Test #
# Version: alpha 0.1                #
# Thomas Kaunzinger                 #
# April 30, 2018                    #
#####################################

import serial
import time

com_port = 'COM3'

serial_port = serial.Serial(port=com_port, baudrate=9600)


# Execution
def main():
    serial_port.close()
    while True:
        # send_voltage(0, 1.0)
        print("Data:")
        print(readback_data())
        time.sleep(3)


# I have to keep looking at the documentation. Supposed to send the DAC address and desired voltage through the COM port
#   and tell the arduino what voltage it wants to send to the DAC.
def send_voltage(dac_address, voltage):
    serial_port.open()
    serial_port.write(dac_address)
    serial_port.write(voltage)
    serial_port.close()


# Reads back the data being sent by the arduino through the serial port
def readback_data():
    serial_port.open()
    return str(serial_port.readline())
    serial_port.close()

# Execute main method
if __name__ == "__main__":
    main()
    print()
    input("Press <Enter> to close...")