#########################################
# DAC Driver Controller                 #
# Version: Beta 0.2                     #
# Thomas Kaunzinger                     #
# May 18, 2018                          #
#                                       #
# A library for interfacing from the    #
# GUI to the communication library with #
# appropriate bridigng calculations.    #
#########################################

###################################################

###########
# IMPORTS #
###########

import serial
from pyduino import *
import pyduino


###################################################

#############
# FUNCTIONS #
#############

# Sends a voltage command
def send_voltage(address: chr, desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool):
    send_command(make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar))


# Sends a setup command
def send_initialization(is_bipolar: bool, gain: str):

    # Finite state machine for polarity and gain
    if is_bipolar:
        polarity = BIPOLAR
    else:
        polarity = UNIPOLAR

    if str(gain) == '2.0':
        gain = GAIN_2
    elif str(gain) == '4.0':
        gain = GAIN_4
    elif str(gain) == '4.32':
        gain = GAIN_432

    command = str(DAC_INDICATOR + START + polarity + gain + DONE)
    send_command(command)


# Use to set the COM Port being used
def set_com(port: str):
    pyduino.serial_port.close()
    pyduino.serial_port = serial.Serial(port=port, baudrate=9600)


###################################################

#############
# EXECUTION #
#############

# Testing stuff that doesn't normally get run cus it's a library
if __name__ == '__main__':

    pass
