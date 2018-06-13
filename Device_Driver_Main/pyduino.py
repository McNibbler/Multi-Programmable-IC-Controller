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
# Simply put, a command from this library starts with an initial ASCII cha6racter representing the device you wish to
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

##############################
# DEVICE ADDRESS IDENTIFIERS #
##############################

# AD5732 DAC
DAC_INDICATOR = 'D'

# LT2977 PMIC
PMIC_INDICATOR = 'P'

# AD9910 DDS
DDS_INDICATOR = 'd'

############
# EXECUTOR #
############

DONE = '!'

################
# DDS COMMANDS #
################

# What type of programming to the DDS to preform
DDS_CONTROL = 'C'  # Control function registers
DDS_CONTROL_MODES = ['1', '2', '3']

DDS_READ = 'r'  # Not really used yet but who knows?
DDS_WRITE = 'w'

DDS_OUTPUT = 'o'  # Signifies programming some sort of output

# Four modes of operation
DDS_SINGLE_TONE = 's'

DDS_RAMP = 'r'
DDS_RAMP_SETUP = 's'
DDS_FREQUENCY = 'f'
DDS_PHASE = 'p'
DDS_AMPLITUDE = 'a'
DDS_RAMP_DISABLE = 'x'
DDS_RAMP_PARAMETERS = 'p'

DDS_RAM = 'R'

DDS_PARALLEL = 'p'

# Command to load values stored in the buffer into the active registers
DDS_LOAD = 'l'

# Single Tone / RAM profiles
DDS_PROFILES = ['0', '1', '2', '3', '4', '5', '6', '7']


#################
# PMIC COMMANDS #
#################

# Different inputs/outputs from the PMIC
PMIC_OUTPUT = 'o'
PMIC_SENSE = 's'
PMIC_ENABLE = 'e'

################
# DAC COMMANDS #
################

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

try:
    com_port = COM_PORTS_LIST[0]
    serial_port = serial.Serial(port=com_port, baudrate=9600)
except (serial.SerialException, IndexError) as exception:
    COMP_PORTS_LIST = ["No Device Available"]
    serial_port = "none"


###################################################

#####################
# LIBRARY FUNCTIONS #
#####################

#################
# DDS FUNCTIONS #
#################

# Functions for communicating to the DDS
class DDS:

    def __init__(self):
        pass

    @staticmethod
    # Sends the command to signify that the data in the registers needs to be loaded
    def create_load_command():
        load_command = str(DDS_INDICATOR + DDS_LOAD + DONE)
        return load_command

    @staticmethod
    # Creates a command to disable the ramp functionality
    def create_disable_ramp_command():
        disable_command = str(DDS_INDICATOR + DDS_OUTPUT + DDS_RAMP + DDS_RAMP_DISABLE + DONE)
        return disable_command

    @staticmethod
    def create_ramp_setup_command(parameter: chr, start, stop, decrement, increment, rate_n, rate_p):
        working_string = str(DDS_INDICATOR + DDS_OUTPUT + DDS_RAMP + DDS_RAMP_SETUP)
        ramp_setup = DDS.create_ramp_setup_string(parameter, start, stop, decrement, increment, rate_n, rate_p)
        working_string = str(working_string + ramp_setup)
        working_string = str(working_string + DONE)
        return working_string

    @staticmethod
    # Creates a string for the parameters of the DRG setup
    def create_ramp_setup_string(parameter: chr, start, stop, decrement, increment, rate_n, rate_p):
        return ''

    @staticmethod
    # literally just works the same as the single tone because I'm using the same method to take care of it, it's just
    #   that the one parameter that you are ramping can be zero as it will get overridden by the ramp anyways
    def create_ramp_parameters_command(amplitude, ref_amplitude, phase, frequency, freq_sysclk):
        working_string = str(DDS_INDICATOR + DDS_OUTPUT + DDS_RAMP + DDS_RAMP_PARAMETERS)
        parameters = DDS.create_parameters_string(amplitude, ref_amplitude, phase, frequency, freq_sysclk)
        working_string = str(working_string + parameters)
        working_string = str(working_string + DONE)
        return working_string

    @staticmethod
    # Creates a string for a proper single tone command to send to the Arduino
    def create_single_tone_command(amplitude, ref_amplitude, phase, frequency, freq_sysclk):
        working_string = str(DDS_INDICATOR + DDS_OUTPUT + DDS_SINGLE_TONE)
        parameters = DDS.create_parameters_string(amplitude, ref_amplitude, phase, frequency, freq_sysclk)
        working_string = str(working_string + parameters)
        working_string = str(working_string + DONE)
        return working_string

    @staticmethod
    # Calculates the parameter words for setting amplitude, phase, and frequency, and then converts them to a usable
    #   string to send in a command
    def create_parameters_string(amplitude, ref_amplitude, phase, frequency, freq_sysclk):
        amp = str(DDS.calculate_amplitude_binary(amplitude, ref_amplitude))
        phs = str(DDS.calculate_phase_binary(phase))
        freq = str(DDS.calculate_frequency_binary(frequency, freq_sysclk))
        return str(amp + ',' + phs + ',' + freq)

    # Commands for calculating the binary integer equivalents for sending to the registers
    @staticmethod
    def calculate_amplitude_binary(amplitude, ref_amplitude):
        amplitude_scale_factor = DDS.calculate_full_scale_binary(14, amplitude, ref_amplitude)
        return amplitude_scale_factor

    @staticmethod
    # Calculates the phase offset word
    def calculate_phase_binary(degrees):
        phase_offset_word = DDS.calculate_full_scale_binary(16, degrees, 360)
        return phase_offset_word

    @staticmethod
    def calculate_frequency_binary(frequency, freq_sysclk):
        frequency_tuning_word = DDS.calculate_full_scale_binary(32, frequency, freq_sysclk)
        return frequency_tuning_word

    # I made an abstraction so prof Ben Lerner will be happy with me and I can say I'm using what I learned in fundies
    @staticmethod
    def calculate_full_scale_binary(num_of_bits, desired, full_scale):
        bits = 1 << num_of_bits
        fraction = desired / full_scale
        word = int(fraction * bits)
        return word


#################
# DAC FUNCTIONS #
#################

# Really only putting these into their own classes to decouple the devices
class DAC:

    def __init__(self):
        pass

    @staticmethod
    # Returns a formatted string command that can be sent
    def make_voltage_command(address: chr, desired_voltage: float,
                             reference_voltage: float, gain: float, bipolar: bool) -> str:
        instructions = str(DAC_INDICATOR + DAC_WRITE + address)

        data = str(DAC.calculate_bits(desired_voltage, reference_voltage, gain, bipolar))

        command = str(instructions + data + DONE)

        return command

    @staticmethod
    # Calculates the integer for the DAC to use
    def calculate_bits(desired_voltage: float, reference_voltage: float, gain: float, bipolar: bool) -> int:
        if bipolar:
            fraction = (desired_voltage + gain * reference_voltage) / (2 * reference_voltage) / gain
        else:
            fraction = (desired_voltage / reference_voltage) / gain

        # Bitwise operators in python are a goddamn sin i just want my fixed variable sizes why is that a problem smh
        data = int(fraction * (1 << DAC_BITS)) * (1 << (DAC_MAX_BITS - DAC_BITS))

        return data

    @staticmethod
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
