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

# DOCUMENTATION (NEEDS TO BE UPDATED)
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

#######
# DDS #
#######

# Sends a load command to the DDS
def load():
    send_command(DDS.create_load_command())


# Resets the DDS to the defaults I'm using for this program
def reset():
    send_command(DDS.create_reset_command())


# Sends a disable ramp command to the DDS
def disable_ramp():
    send_command(DDS.create_disable_ramp_command())


# Sends a single tone setup command to the DDS
def send_single_tone(amplitude: float, ref_amplitude: float, phase: float, frequency: float, freq_sysclk: float):
    send_command(DDS.create_single_tone_command(amplitude, ref_amplitude, phase, frequency, freq_sysclk))


# Sends the other parameters while in DRG mode (not the ramp setup parameters) (functionally same as send_single_tone())
def send_ramp_parameters(amplitude: float, ref_amplitude: float, phase: float, frequency: float, freq_sysclk: float):
    send_command(DDS.create_ramp_parameters_command(amplitude, ref_amplitude, phase, frequency, freq_sysclk))


# Sends the command to set up the DRG for the desired parameter
def send_ramp_setup(parameter: chr, sysclk, reference, start, stop, decrement, increment, rate_n, rate_p):
    send_command(DDS.create_ramp_setup_command(parameter, sysclk, reference, start, stop, decrement, increment, rate_n, rate_p))


#######
# DAC #
#######

# Sends a voltage command
def send_voltage(address: chr, desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool):
    send_command(DAC.create_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar))


# Sends a setup command
def send_initialization(is_bipolar: bool, gain: str):
    send_command(DAC.create_initialization_command(is_bipolar, gain))


#######
# COM #
#######

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

    # Test command because this wasn't working
    send_ramp_setup('f', 360, 1, 0, 360, 1, 1, 0.0000005, 0.0000005)

