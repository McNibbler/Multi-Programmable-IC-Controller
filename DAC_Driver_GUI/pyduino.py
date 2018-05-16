#########################################
# Pyduino                               #
# Version: Alpha 0.1                    #
# Thomas Kaunzinger                     #
# May 11, 2018                          #
#                                       #
# A small library for the purpose of    #
# sending commands to the Arduino DAC   #
# controller.                           #
#########################################

import serial
import time
from struct import *


#############
# CONSTANTS #
#############

# COMMANDS #
# r/w
READ = 'r'
WRITE = 'w'

# Addresses
DAC_A = 'a'
DAC_B = 'b'
DAC_2 = '2'

# Execution
DONE = '!'

# OTHER CONSTANTS #
# Bit precision of the DAC
BITS = 14
MAX_BITS = 16

# SERIAL SETUP #
# This needs to be able to change
com_port = 'COM4'   # Can be different
serial_port = serial.Serial(port=com_port, baudrate=9600)


#####################
# LIBRARY FUNCTIONS #
#####################

# Sends a voltage command
def send_voltage(address, desired_voltage, reference_voltage, gain, bipolar):
    send_command(make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar))


# Returns a formatted string command that can be sent
def make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar):

    instructions = WRITE
    instructions = str(instructions + address)

    data = calculate_bits(desired_voltage, reference_voltage, gain, bipolar)

    command = (instructions, data)

    # command = WRITE
    # command = command_appender(command, address)
    #
    # data = calculate_bits(desired_voltage, reference_voltage, gain, bipolar)
    #
    # command = command_appender(command, data)
    #
    # command = command_appender(command, DONE)

    return command


# Calculates the bits for the DAC to use
def calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int:

    if bipolar:
        fraction = (desired_voltage + gain * reference_voltage) / (2 * reference_voltage) / gain
    else:
        fraction = (desired_voltage / reference_voltage) / gain

    data = int(fraction * (1 << BITS)) << (MAX_BITS - BITS)

    # result = []  # bytearray()
    # bytes = 2
    # mask = 0xFF

    # for i in range(0, bytes):
    #     result.append(data & mask)
    #     data >>= 8

    # result.reverse()

    mask = 0xFFFF
    short_data = data & mask

    return short_data


# # Takes a command and appends a character byte to it
# def command_appender(working_string: str, append: str or int):
#
#     if type(append) is str:
#         return str(working_string + append)
#
#     return str(working_string + chr(append))


#########################
# LOWER-LEVEL FUNCTIONS #
#########################

# Sends a written command
def send_command(command: tuple):
    # serial_port.write(command[0].encode())

    data_first = command[1] >> 8
    data_second = command[1] - (data_first << 8)

    # print(data_first)
    # print(data_second)

    # serial_port.write(data_first)
    # serial_port.write(data_second)

    packed_data = pack('ccHc', 'w'.encode(), '2'.encode() , command[1], DONE.encode())
    print(packed_data)

    serial_port.write(packed_data)
    print(bin(ord(serial_port.read().decode())))
    print(bin(ord(serial_port.read().decode())))
    print(bin(ord(serial_port.read().decode())))


#############
# EXECUTION #
#############

# Testing stuff that doesn't normally get run cus it's a library
if __name__ == '__main__':

    desire = 4.9999999
    ref = 2.5
    gain = 2
    bi = True

    time.sleep(2)

    print(make_voltage_command(DAC_2, desire, ref, gain, bi))
    send_voltage(DAC_2, desire, ref, gain, bi)

    print(bin(calculate_bits(desire, ref, gain, bi)))


