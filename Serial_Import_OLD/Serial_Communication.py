#####################################
# Arduino Serial Communication Test #
# Version: alpha 0.4                #
# Thomas Kaunzinger                 #
# May 9, 2018                       #
#                                   #
# A dumb program used to test       #
# serial communication between the  #
# computer and Arduino for use      #
# in future applications.           #
#####################################

import serial
import sys

com_port = 'COM4'   # Can be different
serial_port = serial.Serial(port=com_port, baudrate=9600)


# Execution
def main():
    print("1: valid command, 2: invalid command, 3: custom command, 4: close port, 5: open port, q: quit")
    while True:
        command_choice = input("Choice: ")
        if command_choice == '1':
            serial_port.write(b'waaa!')                         # 1     -       Valid Command
        elif command_choice == '2':
            serial_port.write(b'qqqq!')                         # 2     -       Invalid Command
        elif command_choice == '3':
            custom = input("Type a command: ")                  # 3     -       Custom Command
            serial_port.write(custom.encode())
        elif command_choice == '4':
            serial_port.close()                                 # 4     -       Close Serial Port
        elif command_choice == '5':
            serial_port.open()                                  # 5     -       Open Serial Port
        elif command_choice == 'q' or command_choice == 'Q':
            serial_port.close()                                 # q/Q   -       Quit
            sys.exit()


# To be implemented later

'''
# I have to keep looking at the documentation. Supposed to send the DAC address and desired voltage through the COM port
#   and tell the arduino what voltage it wants to send to the DAC.
def send_voltage(dac_address, voltage):
    serial_port.write(dac_address)
    serial_port.write(voltage)


# Reads back the data being sent by the arduino through the serial port
def readback_data():
    # Serial port recieves the string encoded in binary, thus needing decode(). Arduino also sends newline characters,
    # which we do not need, so that's why I've trimmed the string
    return str(serial_port.readline().decode()[:-2])
'''

# Execute main method
if __name__ == "__main__":
    main()
    print()
    input("Press <Enter> to close...")

