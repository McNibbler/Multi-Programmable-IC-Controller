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
 *  register (default is 5V unipolar range).
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
//////////////////////////////////////////////////////////////////////

///////////////
// CONSTANTS //
///////////////

// Pins
//const int_fast8_t SS = 10;
//const int_fast8_t SDI = 11;
//const int_fast8_t SDO = 12;
//const int_fast8_t CLK = 13;
const int_fast8_t LDAC = 8;

// DAC Information
const int_fast8_t MAX_BITS = 16;          // Number of Bits after the initial header
const int_fast8_t BITS = 14;              // Number of Bits of precision for the DAC being used
const int_fast16_t CLOCK_SPEED = 10000;    // DAC is rated for 30MHz, Arduino clock is much lower? Signal started looking really bad @100kHz

// SPI settings
SPISettings DEFAULT_SETTINGS(CLOCK_SPEED, MSBFIRST, SPI_MODE2);


///////////////////////////
// DAC CONTROL CONSTANTS //
///////////////////////////

// R/W (write is active low)
const uint8_t READ_BIN = 1;
const uint8_t WRITE_BIN = 0;

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
const uint8_t UNI_5_BIN = 0;    // 000
const uint8_t UNI_10_BIN= 1;   // 001
const uint8_t UNI_108_BIN = 2;  // 010
const uint8_t BI_5_BIN = 3;     // 011
const uint8_t BI_10_BIN = 4;    // 100
const uint8_t BI_108_BIN = 5;   // 101

// DAC's gain
const double GAIN = 2;


/////////////////////////
// DAC DRIVER COMMANDS //
/////////////////////////

// Indicates that I'm actually talking about the AD DAC
const uint8_t DAC_INDICATOR = 'D';

// Constant bytes that represent the characters being sent as commands
const uint8_t READ = 'r';
const uint8_t WRITE = 'w';

const uint8_t DAC_A = 'a';
const uint8_t DAC_B = 'b';
const uint8_t DAC_2 = '2';

const uint8_t START = 's';
const uint8_t BIPOLAR = 'b';
const uint8_t UNIPOLAR = 'u';

const uint8_t GAIN_2 = '1';
const uint8_t GAIN_4 = '2';
const uint8_t GAIN_432 = '3';

const uint8_t DONE = '!';


//////////////////////////////////////////////////////////////////////

///////////
// SETUP //
///////////

void setup() {
  
  // Initialization
  digitalWrite(SS, HIGH);   // Set up slave-select pin. SPI lib handles others
  pinMode(LDAC, OUTPUT);    // Sets LDAC to low because synchronous updating isn't important for this
  digitalWrite(LDAC, LOW);
  Serial.begin(9600);       // Starts serial communication through USB for debugging
  SPI.begin();              // Initializes the SPI protocol

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
    dacCommand(command);
  }
  else{
    purge(command);
    return;
  }
}


void dacCommand(QueueArray <uint8_t> &command){
  
  uint8_t header;  // Initializes the data to send
  uint8_t rw;
  uint16_t data;

  if (command.front() == START){
    command.pop();
    uint8_t polarity = command.pop();
    uint8_t gain = command.pop();
    runSetup(polarity, gain);
    purge(command);
    return;
  }
  else if (command.front() == READ)   {rw = READ_BIN;}
  else if (command.front() == WRITE)  {rw = WRITE_BIN;}
  else{
    purge(command);
    return;
  }

  // Clears front
  command.pop();

  // Appends the DAC address to the header. Purges and ignores with with invalid syntax.
  if      (command.front() == DAC_A)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_A_BIN);}
  else if (command.front() == DAC_B)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_B_BIN);}
  else if (command.front() == DAC_2)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_2_BIN);}
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

  sendData(header, data, DEFAULT_SETTINGS);   // Function to send the data to the DAC. Only reaches here if the whole command is valid.
  loadDataDAC();
  
}


// Empties a queue by popping everything in it. There's probably a faster way to do this.
void purge(QueueArray <uint8_t> &queue){
  while(!queue.isEmpty()) queue.pop();
}


///////////////////////
// SPI DATA HANDLING //
///////////////////////

// sends 24-bit sequence to the DAC
void sendData(uint8_t header, uint16_t data, SPISettings settings){
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);
  SPI.transfer(header);
  SPI.transfer16(data);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();
  
  delayMicroseconds(30);        // Mostly arbitrary, but it's a good amount of delay relative to everything
}


// returns an 8 bit header to send to the DAC before the data
uint8_t headerConstructor(uint8_t readWrite, uint8_t dacRegister, uint8_t channel){
  uint8_t header;
  
  header = readWrite << 7;      // RW logic bit
  header += 0 << 6;             // Reserved 0
  header += dacRegister << 3;   // Register for what you are sending data to
  header += channel;            // Which channel of the register are you controlling

  return header;
}

void runSetup(uint8_t polarity, uint8_t gain_mode){

  // makes sure that the data is valid
  if (polarity != BIPOLAR && polarity != UNIPOLAR){
    return;
  }

  // Sets up output range as bipolar or unipolar
  uint8_t rangeHeaderA = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_A_BIN);        // I have to write to all 3 of these channels individually.
  uint8_t rangeHeaderB = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_B_BIN);        // Writing to "BOTH" apparently isn't enough smh.
  uint8_t rangeHeaderBoth = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_2_BIN);
  if (polarity == BIPOLAR){
    if(gain_mode == GAIN_2){
      sendData(rangeHeaderA, BI_5_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, BI_5_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, BI_5_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == GAIN_4){
      sendData(rangeHeaderA, BI_10_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, BI_10_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, BI_10_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == GAIN_432){
      sendData(rangeHeaderA, BI_108_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, BI_108_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, BI_108_BIN, DEFAULT_SETTINGS);
    }
    else{
      return;
    }
    
  }
  else{
    if (gain_mode == GAIN_2){
      sendData(rangeHeaderA, UNI_5_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, UNI_5_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, UNI_5_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == GAIN_4){
      sendData(rangeHeaderA, UNI_10_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, UNI_10_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, UNI_10_BIN, DEFAULT_SETTINGS);
    }
    else if (gain_mode == GAIN_432){
      sendData(rangeHeaderA, UNI_108_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderB, UNI_108_BIN, DEFAULT_SETTINGS);
      sendData(rangeHeaderBoth, UNI_108_BIN, DEFAULT_SETTINGS);
    }
    else{
      return;
    }
  }


  // Sets up DAC preferences
  uint8_t controlToggleHeader = headerConstructor(WRITE_BIN, CONTROL_REGISTER_BIN, TOGGLES_BIN);
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
  sendData(controlToggleHeader, controlToggleData, DEFAULT_SETTINGS);


  // Powers up the DAC channels
  uint8_t powerHeader = headerConstructor(WRITE_BIN, POWER_REGISTER_BIN, uint16_t(0));
  /* POWER OPERATION GUIDE
   * 
   * Data bits are as follows:
   * X     X     X     X     X     0     OCb   X     OCa   X     TSD   X     X     PUb   X     PUa
   * 
   * X = Don't care, I'm defaulting to 0 for this
   * OCa and OCb are read-only bits that alert if overcurrent is detected in either respective DAC channels
   * TSD is a read-only bit and is set in the event that the channels have shut down due to overheating
   * 
   * PUa and PUb are bits to send to power-up their respective DAC channels
   * 
   * To power up both DACs, I will send as follows:
   * 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1
   */
  uint16_t powerData = 5;
  sendData(powerHeader, powerData, DEFAULT_SETTINGS);
  

  // Sends the function to update and load the DAC data
  uint8_t loadHeader = headerConstructor(WRITE_BIN, CONTROL_REGISTER_BIN, LOAD_BIN);
  sendData(loadHeader, uint16_t(0), DEFAULT_SETTINGS);


  // Loads up what's in the buffer
  loadDataDAC();


  // Slight delay
  delay(100);  
  
}


// Sends the function to update and load the DAC data
void loadDataDAC (){
  uint8_t loadHeader = headerConstructor(WRITE_BIN, CONTROL_REGISTER_BIN, LOAD_BIN);
  sendData(loadHeader, uint16_t(0), DEFAULT_SETTINGS);
}





