#########################################
# DAC Driver Controller                 #
# Version: Beta 0.3                     #
# Thomas Kaunzinger                     #
# May 25, 2018                          #
#                                       #
# A library for interfacing from the    #
# GUI to the communication library with #
# appropriate bridigng calculations.    #
#########################################

# DOCUMENTATION
#
# send_voltage()
#   (address: chr, desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> void
#   Send a desired voltage to write to a chosen output of the DAC
#
# send_initialization()
#   (is_bipolar: bool, gain: str) -> void
#   Sends a command to initialize the DAC given the desired settings
#
# set_com()
#   (port: str) -> void
#   Changes the default COM port

###################################################

###########
# IMPORTS #
###########

import pyduino
import serial
from pyduino import *

###################################################

#############
# FUNCTIONS #
#############

# Sends a voltage command
def send_voltage(address: chr, desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool):
    send_command(make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar))


# Sends a setup command
def send_initialization(is_bipolar: bool, gain: str):
    pyduino.send_initialization(is_bipolar, gain)


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
