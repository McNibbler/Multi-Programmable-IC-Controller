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

###########
# IMPORTS #
###########

import serial.tools.list_ports


#############
# CONSTANTS #
#############

# COMMANDS #
# Device indicator
DAC_INDICATOR = 'D'

# r/w
READ = 'r'
WRITE = 'w'

# Addresses
DAC_A = 'a'
DAC_B = 'b'
DAC_2 = '2'

# Setup commands
START = 's'
BIPOLAR = 'b'
UNIPOLAR = 'u'
GAIN_2 = '1'
GAIN_4 = '2'
GAIN_432 = '3'

# Execution
DONE = '!'

# OTHER CONSTANTS #
# Bit precision of the DAC
BITS = 14
MAX_BITS = 16

# SERIAL SETUP #
# This needs to be able to change

com_port = 'COM4'   # Default

# Finds the last com port with the word "arduino" in the device settings and defaults that instead
ports = list(serial.tools.list_ports.comports())

COM_PORTS_LIST = []
for p in ports:
    COM_PORTS_LIST.append(p.device)

serial_port = serial.Serial(port=com_port, baudrate=9600)


#####################
# LIBRARY FUNCTIONS #
#####################

# Returns a formatted string command that can be sent
def make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar):

    instructions = str(DAC_INDICATOR + WRITE + address)

    data = str(calculate_bits(desired_voltage, reference_voltage, gain, bipolar))

    command = str(instructions + data + DONE)

    return command


# Calculates the bits for the DAC to use
def calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int:

    if bipolar:
        fraction = (desired_voltage + gain * reference_voltage) / (2 * reference_voltage) / gain
    else:
        fraction = (desired_voltage / reference_voltage) / gain

    data = int(fraction * (1 << BITS)) * (1 << (MAX_BITS - BITS))

    print(data)

    return data


#########################
# LOWER-LEVEL FUNCTIONS #
#########################

# Sends a written command
def send_command(command: str):

    print(command)
    serial_port.write(command.encode())


#############
# EXECUTION #
#############

# Testing stuff that doesn't normally get run cus it's a library
if __name__ == '__main__':

    pass

