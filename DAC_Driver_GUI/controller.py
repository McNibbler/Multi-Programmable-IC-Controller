#########################################
# DAC Driver Controller                 #
# Version: Alpha 0.1                    #
# Thomas Kaunzinger                     #
# May 11, 2018                          #
#                                       #
# A library for interfacing from the    #
# GUI to the communication library with #
# appropriate bridigng calculations.    #
#########################################

###########
# IMPORTS #
###########

from abc import ABC
from pyduino import *
import time


#############
# FUNCTIONS #
#############

# Sends a voltage command
def send_voltage(address, desired_voltage, reference_voltage, gain, bipolar):
    send_command(make_voltage_command(address, desired_voltage, reference_voltage, gain, bipolar))


if __name__ == '__main__':

    desire = 3
    ref = 2.5
    gain = 2
    bi = True

    time.sleep(2)

    send_voltage(DAC_2, desire, ref, gain, bi)
