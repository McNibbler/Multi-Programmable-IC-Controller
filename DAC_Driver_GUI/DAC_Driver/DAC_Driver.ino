/* DAC Controller (GUI Interfaced Controller)
 * Version: 0.1
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
const int8_t READ_BIN = 1;
const int8_t WRITE_BIN = 0;

// Registers for controlling DAC
const int8_t DAC_REGISTER_BIN = 0;      // 000
const int8_t RANGE_REGISTER_BIN = 1;    // 001
const int8_t POWER_REGISTER_BIN = 2;    // 010
const int8_t CONTROL_REGISTER_BIN = 3;  // 011

// DAC channel addresses
const int8_t DAC_A_BIN = 0;     // 000
const int8_t DAC_B_BIN = 2;     // 010
const int8_t DAC_2_BIN = 4;     // 100

// Control channel addresses
const int8_t NOP_BIN = 0;       // 000
const int8_t TOGGLES_BIN = 1;   // 001
const int8_t CLR_BIN = 4;       // 100
const int8_t LOAD_BIN = 5;      // 101

// Output range select register
const int8_t UNI_5_BIN = 0;    // 000
const int8_t UNI_10_BIN= 1;   // 001
const int8_t UNI_108_BIN = 2;  // 010
const int8_t BI_5_BIN = 3;     // 011
const int8_t BI_10_BIN = 4;    // 100
const int8_t BI_108_BIN = 5;   // 101

// DAC's gain
const double GAIN = 2;


/////////////////////////
// DAC DRIVER COMMANDS //
/////////////////////////

// Constant bytes that represent the characters being sent as commands
const int8_t READ = 'r';
const int8_t WRITE = 'w';

const int8_t DAC_A = 'a';
const int8_t DAC_B = 'b';
const int8_t DAC_2 = '2';

const int8_t START = 's';
const int8_t BIPOLAR = 'b';
const int8_t UNIPOLAR = 'u';

const int8_t DONE = '!';


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
QueueArray <int8_t> currentCommand;

void loop() {

  int8_t newDataEntry;
  
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

void executeCommand(QueueArray <int8_t> &command){
  int8_t header;  // Initializes the data to send
  int8_t rw;
  int16_t data;

  if (command.front() == START){
    command.pop();
    int8_t polarity = command.pop();
    runSetup(polarity);
    purge(command);
    return;
  }
  else if (command.front() == READ)   {rw = READ_BIN;}
  else if (command.front() == WRITE)  {rw = WRITE_BIN;}
  else{
    purge(command);
    return;
  }

  command.pop();

  // Appends the DAC address to the header. Purges and ignores with with invalid syntax.
  if      (command.front() == DAC_A)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_A_BIN);}
  else if (command.front() == DAC_B)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_B_BIN);}
  else if (command.front() == DAC_2)  {header = headerConstructor(rw, DAC_REGISTER_BIN, DAC_2_BIN);}
  else{
    purge(command);
    return;
  }

  command.pop();  // Clears out the already executed piece of the command

  data += command.pop() << 8;   // First 8 bits of the data
  data += command.pop();        // Rest of the 8 bits of the data
  purge(command);               // Purges whatever is left of the command

  sendData(header, data, DEFAULT_SETTINGS);   // Function to send the data to the DAC. Only reaches here if the whole command is valid.
  
  Serial.println("data loaded");
}


// Empties a queue by popping everything in it. There's probably a faster way to do this.
void purge(QueueArray <int8_t> &queue){
  while(!queue.isEmpty()) queue.pop();
}


///////////////////////
// SPI DATA HANDLING //
///////////////////////

// sends 24-bit sequence to the DAC
void sendData(int8_t header, int16_t data, SPISettings settings){
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);
  SPI.transfer(header);
  SPI.transfer16(data);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();
  
  delayMicroseconds(30);        // Mostly arbitrary, but it's a good amount of delay relative to everything
}


// returns an 8 bit header to send to the DAC before the data
int8_t headerConstructor(int8_t readWrite, int8_t dacRegister, int8_t channel){
  int8_t header;
  
  header = readWrite << 7;      // RW logic bit
  header += 0 << 6;             // Reserved 0
  header += dacRegister << 3;   // Register for what you are sending data to
  header += channel;            // Which channel of the register are you controlling

  return header;
}

void runSetup(int8_t polarity){

  // makes sure that the data is valid
  if (polarity != BIPOLAR && polarity != UNIPOLAR){
    Serial.println("Not bi or uni");
    return;
  }

  // Sets up output range as bipolar or unipolar
  int8_t rangeHeaderA = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_A_BIN);        // I have to write to all 3 of these channels individually.
  int8_t rangeHeaderB = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_B_BIN);        // Writing to "BOTH" apparently isn't enough smh.
  int8_t rangeHeaderBoth = headerConstructor(WRITE_BIN, RANGE_REGISTER_BIN, DAC_2_BIN);
  if (polarity == BIPOLAR){
    sendData(rangeHeaderA, BI_5_BIN, DEFAULT_SETTINGS);
    sendData(rangeHeaderB, BI_5_BIN, DEFAULT_SETTINGS);
    sendData(rangeHeaderBoth, BI_5_BIN, DEFAULT_SETTINGS);
  }
  else{
    sendData(rangeHeaderA, UNI_5_BIN, DEFAULT_SETTINGS);
    sendData(rangeHeaderB, UNI_5_BIN, DEFAULT_SETTINGS);
    sendData(rangeHeaderBoth, UNI_5_BIN, DEFAULT_SETTINGS);
  }


  // Sets up DAC preferences
  int8_t controlToggleHeader = headerConstructor(WRITE_BIN, CONTROL_REGISTER_BIN, TOGGLES_BIN);
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
  int16_t controlToggleData = 4;
  sendData(controlToggleHeader, controlToggleData, DEFAULT_SETTINGS);


  // Powers up the DAC channels
  int8_t powerHeader = headerConstructor(WRITE_BIN, POWER_REGISTER_BIN, int16_t(0));
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
  int16_t powerData = 5;
  sendData(powerHeader, powerData, DEFAULT_SETTINGS);
  

  // Sends the function to update and load the DAC data
  int8_t loadHeader = headerConstructor(WRITE_BIN, CONTROL_REGISTER_BIN, LOAD_BIN);
  sendData(loadHeader, int16_t(0), DEFAULT_SETTINGS);

  Serial.println("startup loaded");


  // Slight delay
  delay(100);  
  
}








