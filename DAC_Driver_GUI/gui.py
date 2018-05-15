#####################################
# DAC Driver GUI                    #
# Version: Alpha 0.1                #
# Thomas Kaunzinger                 #
# May 11, 2018                      #
#                                   #
# A GUI for interfacing with the    #
# Arduino DAC controller.           #
#####################################

###################################################

#######################
# IMPORTING LIBRARIES #
#######################

import sys
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *


###################################################

########################
# QT GUI FUNCTIONALITY #
########################

class Application(QWidget):

    # Init me
    def __init__(self):
        super().__init__()

        # Status text for the program
        self.status_text = QLabel()
        self.status_text.setText('Welcome!')

        # DAC initialization button
        self.setup_button = QPushButton('Initialize DAC')
        self.setup_button.setToolTip('Press to initialize the DAC')
        self.setup_button.clicked.connect(self.setup)

        # Selects bipolar mode
        self.bipolar_checkbox = QCheckBox('Bipolar Range')
        self.bipolar_checkbox.toggle()
        self.bipolar_checkbox.setToolTip('Toggle between monopolar and bipolar voltage ranges')
        self.bipolar_checkbox.stateChanged.connect(self.bipolar_toggle)

        # Connects the outputs together
        self.connect_sliders_checkbox = QCheckBox('Tie Outputs')
        self.connect_sliders_checkbox.setToolTip('Make the voltage of DAC B be the same as DAC A')
        self.connect_sliders_checkbox.stateChanged.connect(self.tie_outputs)

        # Voltage sliders
        self.iterator = 10000
        self.bipolar_range = range(-5*self.iterator, 5*self.iterator + 1, 1)
        self.unipolar_range = range(0*self.iterator, 5*self.iterator + 1, 1)

        self.voltage_label_a = QLabel()
        self.voltage_label_a.setText('DAC A')
        self.voltage_textbox_a = QLineEdit()
        self.voltage_textbox_a.setText(str(0.0))
        self.voltage_textbox_a.returnPressed.connect(self.update_sliders)
        self.voltage_slider_a = QSlider(Qt.Horizontal)
        self.voltage_slider_a.setRange(min(self.bipolar_range), max(self.bipolar_range))
        self.voltage_slider_a.valueChanged[int].connect(self.change_voltage)
        self.voltage_slider_a.sliderReleased.connect(self.send_slider)

        self.voltage_label_b = QLabel()
        self.voltage_label_b.setText('DAC B')
        self.voltage_textbox_b = QLineEdit()
        self.voltage_textbox_b.setText(str(0.0))
        self.voltage_textbox_b.returnPressed.connect(self.update_sliders)
        self.voltage_slider_b = QSlider(Qt.Horizontal)
        self.voltage_slider_b.setRange(min(self.bipolar_range), max(self.bipolar_range))
        self.voltage_slider_b.valueChanged[int].connect(self.change_voltage)
        self.voltage_slider_b.sliderReleased.connect(self.send_slider)

        # Input text boxes for voltage
        self.only_double = QDoubleValidator()
        self.voltage_textbox_a.setValidator(self.only_double)
        self.voltage_textbox_b.setValidator(self.only_double)

        # Readback
        self.readback_label = QLabel()
        self.readback_label.setText('Voltage Readback:')
        self.readback_a = QLabel()
        self.readback_a.setText(str('DAC A: ' + str(0.0) + 'V'))
        self.readback_b = QLabel()
        self.readback_b.setText(str('DAC B: ' + str(0.0) + 'V'))

        # Window dimensions
        self.WINDOW_SIZE = (300, 200)
        self.setFixedSize(self.WINDOW_SIZE[0], self.WINDOW_SIZE[1])
        self.setWindowTitle('DAC Controller')

        self.is_bipolar = True
        self.is_tied = False

        self.main_window()

    # Main window execution and layout
    def main_window(self):
        # Layout
        layout = QGridLayout()
        self.setLayout(layout)

        layout.addWidget(self.status_text, 0, 0, 1, 2)
        layout.addWidget(self.bipolar_checkbox, 0, 2, 1, 1)
        layout.addWidget(self.connect_sliders_checkbox, 0, 3, 1, 1)

        layout.addWidget(self.setup_button, 1, 0, 1, 4)

        layout.addWidget(self.voltage_label_a, 2, 0, 1, 3)
        layout.addWidget(self.voltage_textbox_a, 2, 3, 1, 1)

        layout.addWidget(self.voltage_slider_a, 3, 0, 1, 4)

        layout.addWidget(self.voltage_label_b, 4, 0, 1, 3)
        layout.addWidget(self.voltage_textbox_b, 4, 3, 1, 1)

        layout.addWidget(self.voltage_slider_b, 5, 0, 1, 4)

        layout.addWidget(self.readback_label, 6, 0, 1, 4)

        layout.addWidget(self.readback_a, 7, 0, 1, 2)
        layout.addWidget(self.readback_b, 7, 3, 1, 2)

        self.show()

    # Ties the two sliders together
    def tie_outputs(self):

        if self.is_tied:
            self.is_tied = False

            self.voltage_textbox_b.setEnabled(True)
            self.voltage_slider_b.setEnabled(True)
            self.voltage_label_b.setText('DAC B')

        else:
            self.is_tied = True

            self.voltage_textbox_b.setEnabled(False)
            self.voltage_slider_b.setEnabled(False)
            self.voltage_label_b.setText('DAC B (Tied to DAC A)')

    # Select between bipolar mode and unipolar
    def bipolar_toggle(self):

        if self.is_bipolar:
            self.is_bipolar = False
            self.voltage_slider_a.setRange(min(self.unipolar_range), max(self.unipolar_range))
            self.voltage_slider_b.setRange(min(self.unipolar_range), max(self.unipolar_range))
        else:
            self.is_bipolar = True
            self.voltage_slider_a.setRange(min(self.bipolar_range), max(self.bipolar_range))
            self.voltage_slider_b.setRange(min(self.bipolar_range), max(self.bipolar_range))

        self.voltage_textbox_a.setText(str(0.0))
        self.voltage_textbox_b.setText(str(0.0))
        self.voltage_slider_a.setSliderPosition(0)
        self.voltage_slider_b.setSliderPosition(0)

    # Handler for when the slider is changed
    def change_voltage(self):

        new_voltage_a = self.voltage_slider_a.value() / self.iterator
        new_voltage_b = self.voltage_slider_b.value() / self.iterator

        self.voltage_textbox_a.setText(str(new_voltage_a))
        self.voltage_textbox_b.setText(str(new_voltage_b))

    # Handler for when the text is changed and the sliders need to be updated
    def update_sliders(self):

        new_voltage_a = float(self.voltage_textbox_a.text())
        new_voltage_b = float(self.voltage_textbox_b.text())

        if self.is_bipolar:
            if new_voltage_a > 5 or new_voltage_b > 5 or new_voltage_a < -5 or new_voltage_b < -5:
                self.status_text.setText('Bad Input')
                return
        else:
            if new_voltage_a > 5 or new_voltage_b > 5 or new_voltage_a < 0 or new_voltage_b < 0:
                self.status_text.setText('Bad Input')
                return

        self.voltage_slider_a.setValue(int(new_voltage_a * self.iterator))
        self.voltage_slider_b.setValue(int(new_voltage_b * self.iterator))

        if self.is_tied:
            # WRITE CODE HERE TO SEND VOLTAGE TO THE DACS THROUGH THE 'BOTH' REGISTER
            pass
        else:
            # WRITE CODE HERE TO SEND VOLTAGE TO THE DACS THROUGH THE INDIVIDUAL REGISTERS
            pass

    # Sends the setup command to the DAC
    def setup(self):
        # WRITE CODE HERE TO SEND THE SETUP PROCESS TO THE DAC
        pass

    # Sends update commands to the DAC upon releasing the slider
    def send_slider(self):
        if self.is_tied:
            # WRITE CODE HERE TO SEND VOLTAGE TO THE DACS THROUGH THE 'BOTH' REGISTER
            pass
        else:
            # WRITE CODE HERE TO SEND VOLTAGE TO THE DACS THROUGH THE INDIVIDUAL REGISTERS
            pass


###################################################

#############
# EXECUTION #
#############

# Execute me
if __name__ == '__main__':
    app = QApplication(sys.argv)

    nice = Application()

    sys.exit(app.exec_())
