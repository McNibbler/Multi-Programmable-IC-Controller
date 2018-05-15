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

##### COMMANDS #####
# r/w
READ = 'r'
WRITE = 'w'

# Addresses
DAC_A = 'a'
DAC_B = 'b'
DAC_2 = '2'

# Execution
DONE = '!'

##### OTHER CONSTANTS #####
# Bit precision of the DAC
BITS = 14
MAX_BITS = 16


#####################
# LIBRARY FUNCTIONS #
#####################

def send_voltage(address, desired_voltage, reference_voltage, gain, bipolar):

    data_bytes = calculate_bits(desired_voltage, reference_voltage, gain, bipolar)


# Calculates the bits for the DAC to use
def calculate_bits(desired_voltage, reference_voltage, gain, bipolar):

    if bipolar:
        fraction = ((desired_voltage + reference_voltage) / (2 * reference_voltage)) / gain
    else:
        fraction = (desired_voltage / reference_voltage) / gain

    data = int(fraction * (2 ** BITS)) << 2
    data_byte_1 = data >> 8
    data_byte_2 = data - (data_byte_1 << 8)

    return[data_byte_1, data_byte_2]


def command_appender():
    pass


if __name__ == '__main__':

    desire = 1.25
    ref = 2.5
    gain = 2
    bi = True

    print(bin(calculate_bits(desire, ref, gain, bi)[0]) + '    ' + bin(calculate_bits(desire, ref, gain, bi)[1]))


