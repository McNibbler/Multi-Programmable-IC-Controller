/* DAC Controller (GUI Interfaced Controller)
 * Version: Alpha 0.1
 * 
 * Thomas Kaunzinger
 * Xcerra Corp.
 * May 10, 2018
 * 
 * This program is designed to select and output a desired voltage from an AD5722/AD5732/AD5752 DAC using SPI
 * http://www.analog.com/media/en/technical-documentation/data-sheets/AD5722_5732_5752.pdf
 * 
 * This version of the program has additional functionality from the standalone version to interface with a the
 * DAC using simple commands transferredc through the serial port of the Arduino
 */

///// NOTE: MUCH DOCUMENTATION IS COPIED OVER FROM THE STANDALONE VERSION INCLUDING THE SPECIFICS OF THE DAC /////

//////////////////////////
// OPERATION OF THE DAC //
//////////////////////////

/* Power-up sequence:
 *  Ideally, the DAC is to be powered up in the following order to ensure that the DAC registers are loaded with 0x0000:
 *    GND --> SIG_GND --> DAC GND   (I prefer shorting all supplies to the same ground)
 *    DVcc                          (Digital power must be applied BEFORE analog power)
 *    AVss and AVdd                 (Order does not matter for these, as long as they are after DVcc)
 *  
 * Communication:
 *  The DAC is capable of communication through SPI, QSPI, MICROWIRE, and DSP and is rated at 30MHz. Data input is done in
 *  24 bit registers with most significant bit first. This program uses SPI at 10kHz due to limitations of the Arduino.
 * 
 * LDAC:
 *  If this is held high, the DAC waits until the falling edge of LDAC to update all inputs simultaneously. If tied permanently
 *  low, data is updated individually.
 * 
 * CLR:
 *  When CLR is tied permanently low, code input is done through 2's compliment, and while it is tied permanently high, code is
 *  inputted through midscale binary (what this program uses).
 * 
 * First writes:
 *  The first communication to the DAC should be to set the output range on all channels by writing to the output range select
 *  register (default is 5V DAC_UNIPOLAR range).
 *  
 *  Furthermore, to program an output, it must first be powered up using the power control register, otherwise all code trying
 *  to access these is ignored.
 * 
 * Gain:
 *  Internal gain from the DAC is determined by the selected output range from the user.
 *    Range (V)   | Gain
 *    -------------------
 *    + 5         | 2
 *    + 10        | 4
 *    + 10.8      | 4.32
 *    +/- 5       | 4
 *    +/- 10      | 8
 *    +/- 10.8    | 8.64
 */

///////////////
// LIBRARIES //
///////////////

#include <SPI.h>            // SPI communication for the DAC
#include <math.h>           // Math library
#include <stdint.h>         // So I can use nice data structures
#include <QueueArray.h>     // Library for creating command sequences


//////////////////////////////////////////////////////////////////////

///////////////
// CONSTANTS //
///////////////

// Pins
const int_fast8_t SS_DDS = 9;
const int_fast8_t SS_DAC = 10;
//const int_fast8_t SDI = 11;
//const int_fast8_t SDO = 12;
//const int_fast8_t CLK = 13;
const int_fast8_t LDAC = 8;

// SPI settings
const int_fast16_t CLOCK_SPEED = 10000;    // DAC is rated for 30MHz, Arduino clock is much lower? Signal started looking really bad @100kHz
SPISettings DEFAULT_SETTINGS(CLOCK_SPEED, MSBFIRST, SPI_MODE2);


////////////////////////////////
// DEVICE ADDRESS IDENTIFIERS //
////////////////////////////////

// AD5732 DAC
const uint8_t DAC_INDICATOR = 'D';

// LT2977 PMIC
const uint8_t PMIC_INDICATOR = 'P';

// AD9910 DDS
const uint8_t DDS_INDICATOR = 'd';


//////////////
// EXECUTOR //
//////////////
const uint8_t DONE = '!';


//////////////////
// DDS COMMANDS //
//////////////////

  // when u yeet so hard u get yote

  // Four modes of operation
  const uint8_t DDS_SINGLE_TONE = 's';
  const uint8_t DDS_RAM = 'R';
  const uint8_t DDS_RAMP = 'r';
  const uint8_t DDS_RAMP_SETUP = 's';
  const uint8_t DDS_PARALLEL = 'p';

  // What type of programming to the DDS to preform
  const uint8_t DDS_CONTROL = 'C';                        // Control function registers
  const uint8_t DDS_CONTROL_MODES [3] = {'1', '2', '3'};
  const uint8_t DDS_CONTROL_MODES_BIN [3] = {0, 1, 2};
  const uint8_t DDS_OUTPUT = 'o';                         // Signifies programming some sort of output

  // Single tone / RAM profiles
  const uint8_t DDS_PROFILES [8] = {'0', '1', '3', '4', '5', '6', '7'};
  const uint8_t DDS_PROFILES_BIN [8] = {14, 15, 16, 17, 18, 19, 20, 21};
  
  
///////////////////
// PMIC COMMANDS //
///////////////////

  // Whatever
  const uint8_t PMIC_OUTPUT = 'o';
  const uint8_t PMIC_SENSE = 's';
  const uint8_t PMIC_ENABLE = 'e';


//////////////////
// DAC COMMANDS //
//////////////////

  // DAC Information
  const int_fast8_t MAX_BITS = 16;          // Number of Bits after the initial header
  const int_fast8_t BITS = 14;              // Number of Bits of precision for the DAC being used

  // Constant bytes that represent the characters being sent as commands
  const uint8_t DAC_READ = 'r';
  const uint8_t DAC_WRITE = 'w';
  
  const uint8_t DAC_A = 'a';
  const uint8_t DAC_B = 'b';
  const uint8_t DAC_2 = '2';
  
  const uint8_t DAC_START = 's';
  const uint8_t DAC_BIPOLAR = 'b';
  const uint8_t DAC_UNIPOLAR = 'u';
  
  const uint8_t DAC_GAIN_2 = '1';
  const uint8_t DAC_GAIN_4 = '2';
  const uint8_t DAC_GAIN_432 = '3';
  

  ///////////////////////////
  // DAC CONTROL CONSTANTS //
  ///////////////////////////
  
  // R/W (DAC_WRITE is active low)
  const uint8_t DAC_READ_BIN = 1;
  const uint8_t DAC_WRITE_BIN = 0;
  
  // Registers for controlling DAC
  const uint8_t DAC_REGISTER_BIN = 0;      // 000
  const uint8_t RANGE_REGISTER_BIN = 1;    // 001
  const uint8_t POWER_REGISTER_BIN = 2;    // 010
  const uint8_t CONTROL_REGISTER_BIN = 3;  // 011
  
  // DAC channel addresses
  const uint8_t DAC_A_BIN = 0;     // 000
  const uint8_t DAC_B_BIN = 2;     // 010
  const uint8_t DAC_2_BIN = 4;     // 100
  
  // Control channel addresses
  const uint8_t NOP_BIN = 0;       // 000
  const uint8_t TOGGLES_BIN = 1;   // 001
  const uint8_t CLR_BIN = 4;       // 100
  const uint8_t LOAD_BIN = 5;      // 101
  
  // Output range select register
  const uint8_t DAC_UNI_5_BIN = 0;    // 000
  const uint8_t DAC_UNI_10_BIN= 1;   // 001
  const uint8_t DAC_UNI_108_BIN = 2;  // 010
  const uint8_t DAC_BI_5_BIN = 3;     // 011
  const uint8_t DAC_BI_10_BIN = 4;    // 100
  const uint8_t DAC_BI_108_BIN = 5;   // 101


//////////////////////////////////////////////////////////////////////

///////////
// SETUP //
///////////

// Initialization
void setup() {

  // Set up slave-select pins. SPI lib handles others
  pinMode(SS_DDS, OUTPUT);
  pinMode(SS_DAC, OUTPUT);
  digitalWrite(SS_DDS, HIGH);
  digitalWrite(SS_DAC, HIGH);

  // Sets LDAC to low because synchronous updating isn't important for this
  pinMode(LDAC, OUTPUT);
  digitalWrite(LDAC, LOW);

  // Initializes Serial communication through USB for commands
  Serial.begin(9600);
  
  // Initializes the SPI protocol
  SPI.begin();

}


////////////////////
// EXECUTION LOOP //
////////////////////

// Initializes the current command to be executed until the execution byte is sent
QueueArray <uint8_t> currentCommand;

void loop() {

  uint8_t newDataEntry;
  
  while (Serial.available() > 0){

    newDataEntry = Serial.read();
    currentCommand.push(newDataEntry);
  
    // Executes when the termination statement is received
    if (newDataEntry == DONE){
      executeCommand(currentCommand);
      purge(currentCommand);
    }
  }
}


//////////////////////
// COMMAND HANDLING //
//////////////////////

void executeCommand(QueueArray <uint8_t> &command){

  // Expandable so that I could potentially run different execution commands for other devices?
  // PMIC monitoring system may be added.
  if (command.front() == DAC_INDICATOR){
    command.pop();
    DACcommand(command);
    purge(command);
    return;
  }
  else if (command.front() == PMIC_INDICATOR){
    command.pop();
    PMICcommand(command);
    purge(command);
    return;
  }
  else if (command.front() == DDS_INDICATOR){
    command.pop();
    DDScommand(command);
    purge(command);
    return;
  }
  else{
    purge(command);
    return;
  }
}

// Empties a queue by popping everything in it. There's probably a faster way to do this.
void purge(QueueArray <uint8_t> &queue){
  while(!queue.isEmpty()) queue.pop();
}


///////////////////
// DDS COMMANDS //
///////////////////

// Handles the front command to see whether or not it is for control or output
void DDScommand(QueueArray <uint8_t> &command){

  uint8_t front = command.pop();

  if (front == DDS_CONTROL){
    DDScontrolHandler(command);
    purge(command);
    return;
  }
  else if (front == DDS_OUTPUT){
    DDSoutputHandler(command);
    purge(command);
    return;
  }
  else{
    purge(command);
    return;
  }

}

// lol idfk yet bear with me
void DDScontrolHandler(QueueArray <uint8_t> &command){
  purge(command);
  return;
}

// handles the different output possibilities
void DDSoutputHandler(QueueArray <uint8_t> &command){
  uint8_t front = command.pop();

  // Creates a single constant wave of one frequency, amplitude, and phase
  if (front = DDS_SINGLE_TONE){
    purge(command);
    return;
  }
  
  // Creates a digital ramp of differnt frequencies to produce different sine waves
  else if (front = DDS_RAMP){
    if (command.front() == DDS_RAMP_SETUP){
      command.pop();
      DDSrampSetup(command);
      purge(command);
      return;
    }

    String amplitudeString;
    while (command.front() != '!'){
      amplitudeString += (char)command.pop();
    }
    int_fast16_t amplitude = amplitudeString.toInt();
    ramplitude(amplitude);
    purge(command);
    return;
  }

  // Expandable if I want to implement RAM or parallel programming later (I probably won't)
  else if (front = DDS_RAM || front == DDS_PARALLEL){
    purge(command);
    return;
  }

  // Catch exceptions and purge
  else{
    purge(command);
    return;
  }
  
}

// Parses the rest of the command for the setup for the DRG
void DDSrampSetup(QueueArray <uint8_t> &command){
  purge(command);
  return;
}

// I'm so happy that I could name a function this
void ramplitude(int_fast16_t amplitude){
  return;
}


///////////////////
// PMIC COMMANDS //
///////////////////

// To be implemented
void PMICcommand(QueueArray <uint8_t> &command){

  purge(command);
  return;

}


//////////////////
// DAC COMMANDS //
//////////////////

// Runs through and interperates the DAC command after the header
void DACcommand(QueueArray <uint8_t> &command){
  
  uint8_t header;  // Initializes the data to send
  uint8_t rw;
  uint16_t data;

  if (command.front() == DAC_START){
    command.pop();
    uint8_t polarity = command.pop();
    uint8_t gain = command.pop();
    DACrunSetup(polarity, gain);
    purge(command);
    return;
  }
  else if (command.front() == DAC_READ)   {rw = DAC_READ_BIN;}
  else if (command.front() == DAC_WRITE)  {rw = DAC_WRITE_BIN;}
  else{
    purge(command);
    return;
  }

  // Clears front
  command.pop();

  // Appends the DAC address to the header. Purges and ignores with with invalid syntax.
  if      (command.front() == DAC_A)  {header = DACheaderConstructor(rw, DAC_REGISTER_BIN, DAC_A_BIN);}
  else if (command.front() == DAC_B)  {header = DACheaderConstructor(rw, DAC_REGISTER_BIN, DAC_B_BIN);}
  else if (command.front() == DAC_2)  {header = DACheaderConstructor(rw, DAC_REGISTER_BIN, DAC_2_BIN);}
  else{
    purge(command);
    return;
  }

  // Clears front
  command.pop();

  // Converts the recieved string to work into an integer to send as data
  String data_string;
  while (command.front() != DONE){
    data_string += (char)command.pop();
  }
  data = data_string.toInt();

  DACsendData(header, data, DEFAULT_SETTINGS);   // Function to send the data to the DAC. Only reaches here if the whole command is valid.
  DACloadData();
  
}


///////////////////////////
// DAC SPI DATA HANDLING //
///////////////////////////

// sends 24-bit sequence to the DAC
void DACsendData(uint8_t header, uint16_t data, SPISettings settings){
  SPI.beginTransaction(settings);
  digitalWrite(SS_DAC, LOW);
  SPI.transfer(header);
  SPI.transfer16(data);
  digitalWrite(SS_DAC, HIGH);
  SPI.endTransaction();
  
  delayMicroseconds(30);        // Mostly arbitrary, but it's a good amount of delay relative to everything
}


// returns an 8 bit header to send to the DAC before the data
uint8_t DACheaderConstructor(uint8_t readWrite, uint8_t dacRegister, uint8_t channel){
  uint8_t header;
  
  header = readWrite << 7;      // RW logic bit
  header += 0 << 6;             // Reserved 0
  header += dacRegister << 3;   // Register for what you are sending data to
  header += channel;            // Which channel of the register are you controlling

  return header;
}

void DACrunSetup(uint8_t polarity, uint8_t gain_mode){

  // makes sure that the data is valid
  if (polarity != DAC_BIPOLAR && polarity != DAC_UNIPOLAR){
    return;
  }

  // Sets up output range as DAC_BIPOLAR or DAC_UNIPOLAR
  uint8_t rangeHeaderA = DACheaderConstructor(DAC_WRITE_BIN, RANGE_REGISTER_BIN, DAC_A_BIN);        // I have to DAC_WRITE to all 3 of these channels individually.
  uint8_t rangeHeaderB = DACheaderConstructor(DAC_WRITE_BIN, RANGE_REGISTER_BIN, DAC_B_BIN);        // Writing to "BOTH" apparently isn't enough smh.
  uint8_t rangeHeaderBoth = DACheaderConstructor(DAC_WRITE_BIN, RANGE_REGISTER_BIN, DAC_2_BIN);
  if (polarity == DAC_BIPOLAR){
    if(gain_mode == DAC_GAIN_2){
      DACsendData(rangeHeaderA, DAC_BI_5_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_BI_5_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_BI_5_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == DAC_GAIN_4){
      DACsendData(rangeHeaderA, DAC_BI_10_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_BI_10_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_BI_10_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == DAC_GAIN_432){
      DACsendData(rangeHeaderA, DAC_BI_108_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_BI_108_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_BI_108_BIN, DEFAULT_SETTINGS);
    }
    else{
      return;
    }
    
  }
  else{
    if (gain_mode == DAC_GAIN_2){
      DACsendData(rangeHeaderA, DAC_UNI_5_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_UNI_5_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_UNI_5_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == DAC_GAIN_4){
      DACsendData(rangeHeaderA, DAC_UNI_10_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_UNI_10_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_UNI_10_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == DAC_GAIN_432){
      DACsendData(rangeHeaderA, DAC_UNI_108_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderB, DAC_UNI_108_BIN, DEFAULT_SETTINGS);
      DACsendData(rangeHeaderBoth, DAC_UNI_108_BIN, DEFAULT_SETTINGS);
    }
    else{
      return;
    }
  }


  // Sets up DAC preferences
  uint8_t controlToggleHeader = DACheaderConstructor(DAC_WRITE_BIN, CONTROL_REGISTER_BIN, TOGGLES_BIN);
  /* CONTROL TOGGLES OPERATION GUIDE
   * 
   * Thermal SD       0 = No thermal shutdown         1 = Enable thermal shutdown
   * Clamp enable     0 = Auto channel shutdown       1 = Constant current clamp when shorted
   * CLR Select       0 = CLR clears to GND           1 = CLR clears to full-scale
   * SDO disable      0 = Keeps slave data out        1 = Disables slave data out
   * 
   * Thermal SD   | Clamp enable  | CLR Select    | SDO disable
   * ----------------------------------------------------------
   * 1            | 0             | 0             | 0
   */
  uint16_t controlToggleData = 4;
  DACsendData(controlToggleHeader, controlToggleData, DEFAULT_SETTINGS);


  // Powers up the DAC channels
  uint8_t powerHeader = DACheaderConstructor(DAC_WRITE_BIN, POWER_REGISTER_BIN, uint16_t(0));
  /* POWER OPERATION GUIDE
   * 
   * Data bits are as follows:
   * X     X     X     X     X     0     OCb   X     OCa   X     TSD   X     X     PUb   X     PUa
   * 
   * X = Don't care, I'm defaulting to 0 for this
   * OCa and OCb are DAC_READ-only bits that alert if overcurrent is detected in either respective DAC channels
   * TSD is a DAC_READ-only bit and is set in the event that the channels have shut down due to overheating
   * 
   * PUa and PUb are bits to send to power-up their respective DAC channels
   * 
   * To power up both DACs, I will send as follows:
   * 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1
   */
  uint16_t powerData = 5;
  DACsendData(powerHeader, powerData, DEFAULT_SETTINGS);
  

  // Sends the function to update and load the DAC data
  uint8_t loadHeader = DACheaderConstructor(DAC_WRITE_BIN, CONTROL_REGISTER_BIN, LOAD_BIN);
  DACsendData(loadHeader, uint16_t(0), DEFAULT_SETTINGS);


  // Loads up what's in the buffer
  DACloadData();


  // Slight delay
  delay(100);  
  
}


// Sends the function to update and load the DAC data
void DACloadData (){
  uint8_t loadHeader = DACheaderConstructor(DAC_WRITE_BIN, CONTROL_REGISTER_BIN, LOAD_BIN);
  DACsendData(loadHeader, uint16_t(0), DEFAULT_SETTINGS);
}






