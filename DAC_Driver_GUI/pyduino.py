#########################################
# Pyduino                               #
# Version: Beta 0.2                     #
# Thomas Kaunzinger                     #
# May 18, 2018                          #
#                                       #
# A small library for the purpose of    #
# sending commands to the Arduino DAC   #
# controller.                           #
#########################################

###################################################

###########
# IMPORTS #
###########

import serial.tools.list_ports


###################################################

#############
# CONSTANTS #
#############

# COMMANDS #
# Device indicator
DAC_INDICATOR = 'D'

# DAC CONTROLS #
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


###################################################

################
# SERIAL SETUP #
################
# Finds the list of COM Ports available and sets one as the default
ports = list(serial.tools.list_ports.comports())

COM_PORTS_LIST = []
for p in ports:
    COM_PORTS_LIST.append(p.device)

com_port = COM_PORTS_LIST[0]

serial_port = serial.Serial(port=com_port, baudrate=9600)


###################################################

#####################
# LIBRARY FUNCTIONS #
#####################

# Returns a formatted string command that can be sent
def make_voltage_command(address: chr, desired_voltage: float,
                         reference_voltage: float, gain: float, bipolar: bool) -> str:

    instructions = str(DAC_INDICATOR + WRITE + address)

    data = str(calculate_bits(desired_voltage, reference_voltage, gain, bipolar))

    command = str(instructions + data + DONE)

    return command


# Calculates the integer for the DAC to use
def calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int:

    if bipolar:
        fraction = (desired_voltage + gain * reference_voltage) / (2 * reference_voltage) / gain
    else:
        fraction = (desired_voltage / reference_voltage) / gain

    # Bitwise operators in python are a goddamn sin i just want my fixed variable sizes why is that a problem smh
    data = int(fraction * (1 << BITS)) * (1 << (MAX_BITS - BITS))

    return data


###################################################

#########################
# LOWER-LEVEL FUNCTIONS #
#########################

# Sends a written command
def send_command(command: str):
    print(command)
    serial_port.write(command.encode())


###################################################

#############
# EXECUTION #
#############

# Testing stuff that doesn't normally get run cus it's a library
if __name__ == '__main__':

    pass

