# WINDOWS EXE COMPILER PROGRAM

import os
from cx_Freeze import setup, Executable

text_read = open('tcl_tk_location.txt', 'r')
env_locations = text_read.readline()

# C:\\WHERE YOUR PYTHON DIRECTORY IS LOCATED\\Python\\Python36-32\\tcl\\tcl8.6
# C:\\WHERE YOUR PYTHON DIRECTORY IS LOCATED\\Python\\Python36-32\\tcl\\tk8.6
os.environ['TCL_LIBRARY'] = env_locations[0]
os.environ['TK_LIBRARY'] = env_locations[1]

setup(name='DAC Programmer',
      version='0.2.1',
      description='https://github.com/McNibbler/DAC-Controller',
      options={'build_exe': {'packages': ['sys', 'PyQt5', 'serial', 'pyduino', 'controller']}},
      executables=[Executable('gui.py', base='Win32GUI')])

# To build with an MSI installer, run with the argument "bdist_msi"
#
# > python3 setup.py bdist_msi
