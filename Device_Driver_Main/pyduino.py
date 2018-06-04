#########################################
# Pyduino                               #
# Version: Beta 0.3                     #
# Thomas Kaunzinger                     #
# May 25, 2018                          #
#                                       #
# A small library for the purpose of    #
# sending commands to the Arduino DAC   #
# controller.                           #
#########################################

# HOW THIS LIBRARY WORKS
#
# The Pyduino library's purpose is to establish a simple communication command sequence that can be
# sent easily over a serial connection and received by an Arduino or similar device to execute commands based
# on the implementation of the interpretation on the Arduino side.
#
# Simply put, a command from this library starts with an initial ASCII character representing the device you wish to
# communicate to. In this implementation, I have "D" at the front to signify that I'm talking to the DAC connected
# to the arduino. Later on I plan to write some more to control a PMIC and DDS with this same system, so I may
# use something like "P" to say I want to write to a PMIC. This is expandable to write command schemes for more
# devices if you implement them on the arduino side.
#
# Furthermore, all commands end in the reserved character "!" which signifies that the command is complete and
# ready to be sent to your arduino functions to be read and executed.
#
#
# IMPLEMENTED DEVICES
# "D" - AD5722/AD5732/AD5752 DAC
# After initial character, one can either send a setup command or a read/write command
#
# "s": Start command
#   Afterwards, send the configuration for bipolar or unipolar: "b" or "u"
#   Followed by gain mode: x2 = "1", x4 = "2", x4.32 = "3"
#   Finish with a "!"
#   e.g. - "Dsb1!"
#
# "w": Write command
#   Afterwards send a DAC register you wish to write to: A = "a", B = "b", Both = "2"
#   Followed by the INTEGER value of the data you wish to write to that DAC
#       Convert the integer to a characters representing the number.
#       This integer can be created using calculate_bits()
#           calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int
#   Finish with a "!"
#   e.g. - "Dwb12832!"
#
# "r": Read command
#   Afterwards send a DAC register you wish to read from: A = "a", B = "b", Both = "2"
#   Finish with a "!"
#   e.g. - "Dra!"
#   NOTE: Not implemented yet!!!! Requires processing and reading back functions that are not implemented yet!
#
#
# FUNCTIONS
# make_voltage_command()
#   (address: chr, desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> str
#   Returns a string that can be sent as a write command for the DAC to set an addressable output to a desired voltage
#
# calculate_bits()
#   (desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int
#   Returns an integer that can be used in make_voltage_command to send for the data for the DAC
#
# send_initialization()
#   (is_bipolar: bool, gain: str) -> void
#   Creates and sends a setup command to the DAC
#
# send_command()
#   (command: str) -> void
#   Sends the command through the serial COM port

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
DAC_READ = 'r'
DAC_WRITE = 'w'
# Addresses
DAC_A = 'a'
DAC_B = 'b'
DAC_2 = '2'
# Setup commands
DAC_START = 's'
DAC_BIPOLAR = 'b'
DAC_UNIPOLAR = 'u'
DAC_GAIN_2 = '1'
DAC_GAIN_4 = '2'
DAC_GAIN_432 = '3'
# Execution
DONE = '!'

# OTHER CONSTANTS #
# Bit precision of the DAC
DAC_BITS = 14
DAC_MAX_BITS = 16


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

    instructions = str(DAC_INDICATOR + DAC_WRITE + address)

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
    data = int(fraction * (1 << DAC_BITS)) * (1 << (DAC_MAX_BITS - DAC_BITS))

    return data


# Sends a setup command
def send_initialization(is_bipolar: bool, gain: str):

    # Finite state machine for polarity and gain
    if is_bipolar:
        polarity = DAC_BIPOLAR
    else:
        polarity = DAC_UNIPOLAR

    if str(gain) == '2.0':
        gain = DAC_GAIN_2
    elif str(gain) == '4.0':
        gain = DAC_GAIN_4
    elif str(gain) == '4.32':
        gain = DAC_GAIN_432

    command = str(DAC_INDICATOR + DAC_START + polarity + gain + DONE)
    send_command(command)


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

