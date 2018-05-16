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

# Returns a formatted string command that can be sent
def make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar):

    instructions = WRITE
    instructions = str(instructions + address)

    data = calculate_bits(desired_voltage, reference_voltage, gain, bipolar)

    command = (instructions, data)

    return command


# Calculates the bits for the DAC to use
def calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int:

    if bipolar:
        fraction = (desired_voltage + gain * reference_voltage) / (2 * reference_voltage) / gain
    else:
        fraction = (desired_voltage / reference_voltage) / gain

    data = int(fraction * (1 << BITS)) << (MAX_BITS - BITS)

    mask = 0xFFFF
    short_data = data & mask

    return short_data


#########################
# LOWER-LEVEL FUNCTIONS #
#########################

# Sends a written command
def send_command(command: tuple):

    data_first = command[1] >> 8
    data_second = command[1] - (data_first << 8)

    serial_port.write(command[0].encode())

    serial_port.write(chr(data_first).encode())
    serial_port.write(chr(data_second).encode())

    serial_port.write(DONE.encode())


#############
# EXECUTION #
#############

# Testing stuff that doesn't normally get run cus it's a library
if __name__ == '__main__':

    pass

