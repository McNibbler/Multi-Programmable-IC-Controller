/* DAC Controller
 * Version: beta 0.4
 * 
 * Thomas Kaunzinger
 * Xcerra Corp.
 * April 25, 2018
 * 
 * This program is designed to select and output a desired voltage from an AD5722/AD5732/AD5752 DAC using SPI
 * http://www.analog.com/media/en/technical-documentation/data-sheets/AD5722_5732_5752.pdf
 */

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

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

////////////////////
// USER VARIABLES //
////////////////////

// Set REFERENCE_VOLTAGE to the voltage of the reference pin
const double REFERENCE_VOLTAGE = 2.5;

// Set DESIRED_VOLTAGE to the voltage you wish to produce from the DAC
const double DESIRED_VOLTAGE_1 = 1.25;
const double DESIRED_VOLTAGE_2 = 0.75;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

///////////////////////////
// LIBRARIES + CONSTANTS //
///////////////////////////

// Libraries
#include <SPI.h>
#include <math.h>

// Pins
//const int SS = 10;
//const int SDI = 11;
//const int SDO = 12;
//const int CLK = 13;
const int LDAC = 8;

// DAC Information
const int MAX_BITS = 16;          // Number of Bits after the initial header
const int BITS = 14;              // Number of Bits of precision for the DAC being used
const bool BIPOLAR = true;        // Currently, the DAC is set to have a bipolar output
const int CLOCK_SPEED = 10000;    // DAC is rated for 30MHz, Arduino clock is much lower? Signal started looking really bad @100kHz

// SPI settings
// This uses SPI_MODE1 or SPI_MODE2 but I'm not 100% sure which one it is. I think 2 but I can't tell.
SPISettings DEFAULT_SETTINGS(CLOCK_SPEED, MSBFIRST, SPI_MODE2);

// Input pins for DACs to read back voltages
const int DAC_1 = A4;
const int DAC_2 = A5;


///////////////////////////
// DAC CONTROL CONSTANTS //
///////////////////////////

// R/W (write is active low)
const char READ = 1;
const char WRITE = 0;

// Registers for controlling DAC
const char DAC_REGISTER = 0;      // 000
const char RANGE_REGISTER = 1;    // 001
const char POWER_REGISTER = 2;    // 010
const char CONTROL_REGISTER = 3;  // 011

// DAC channel addresses
const char DAC_A = 0;     // 000
const char DAC_B = 2;     // 010
const char DAC_BOTH = 4;  // 100

// Control channel addresses
const char NOP = 0;       // 000
const char TOGGLES = 1;   // 001
const char CLR = 4;       // 100
const char LOAD = 5;      // 101

// Output range select register
const short UNI_5 = 0;    // 000
const short UNI_10 = 1;   // 001
const short UNI_108 = 2;  // 010
const short BI_5 = 3;     // 011
const short BI_10 = 4;    // 100
const short BI_108 = 5;   // 101

// Load header (loads the data)
const char LOAD_HEADER = headerConstructor(WRITE, CONTROL_REGISTER, LOAD);



//////////////////////////////////////////////////////////////////////

///////////
// SETUP //
///////////
void setup() {

  // Initialization
  digitalWrite(SS, HIGH);   // Set up slave-select pin. SPI lib handles others
  pinMode(LDAC, OUTPUT);    // Sets LDAC to low because synchronous updating isn't important for this
  digitalWrite(LDAC, LOW);
  pinMode(DAC_1, INPUT);    // Initializes readback pins for voltages
  pinMode(DAC_2, INPUT);
  Serial.begin(9600);       // Starts serial communication through USB for debugging
  SPI.begin();              // Initializes the SPI protocol


  // I'm generous and giving you 10 whole seconds before starting to power everything
  delay(10000);


  // Sets up output range
  char rangeHeader = headerConstructor(WRITE, RANGE_REGISTER, DAC_BOTH);
  if (BIPOLAR){
    sendData(rangeHeader, BI_5, SETTINGS);
  }
  else{
    sendData(rangeHeader, UNI_5, SETTINGS);
  }


  // Sets up DAC preferences
  char controlToggleHeader = headerConstructor(WRITE, CONTROL_REGISTER, TOGGLES);
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
  short controlToggleData = 4;
  sendData(controlHeader, controlToggleData, SETTINGS);
  

  // Powers up the DAC channels
  char powerHeader = headerConstructor(WRITE, POWER_REGISTER, short(0));
  /* POWER OPERATION GUIDE
   * 
   * Data bits are as follows:
   * 
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
  short powerData = 3;
  sendData(powerHeader, powerData, SETTINGS);


  // Sends the function to update and load the DAC data
  sendData(LOAD_HEADER, short(0), SETTINGS);


  // Slight delay before sending the data to the DAC repeatedly in the loop
  delay(1000);
}


//////////////////////////////////////////////////////////////////////

////////////////////
// EXECUTION LOOP //
////////////////////
void loop() {

  // Sends the desired DAC data
  setOutput(DESIRED_VOLTAGE_1, DESIRED_VOLTAGE_2, REFERENCE_VOLTAGE, DEFAULT_SETTINGS, BIPOLAR);
  
  // Sends the function to update and load the DAC data
  sendData(LOAD_HEADER, short(0), SETTINGS);

  // Prints the data from the two read-in pins for debugging
  Serial.println(analogRead(DAC_1));
  Serial.println(analogRead(DAC_2));
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~");

  // Slight delay between sending data
  delayMicroseconds(500);

}


//////////////////////////////////////////////////////////////////////

//////////////////////
// HELPER FUNCTIONS //
//////////////////////

// Enables the slave select for the DAC, calculates the value for the desired voltage, and sendDatas to DAC through SPI before closing the SS
void setOutput(double desired1, double desired2, double reference, SPISettings settings, bool bipolar){

  // Creates an 8-bit header to send to the chip to show where it is writing its information to
  char header = headerConstructor(0, 0, 0);   // Write is active low, DAC register is 000, dacChannel is set to 0 to initialize

  // Calculates the bits of data to send to the DAC
  short bits1 = calcOutput(desired1, reference, bipolar);
  short bits2 = calcOutput(desired2, reference, bipolar);

  char dacChannel;
  
  // checks if the two variables are exactly the same (e.g. calling setOutput(DESIRED_VOLTAGE_1, DESIRED_VOLTAGE_1, ...)) and uses the 'Both' address
  if (desired1 == desired2){
    
    header += DAC_BOTH;
    sendData(header, bits1, settings);
    
  }


  // Sets DAC A and then DAC B to the two desired voltages
  else{

    header += DAC_A;
    sendData(header, bits1, settings);
    header -= DAC_A; // Removes the address from the first DAC

    header += DAC_B;
    sendData(header, bits2, settings);
    
  }
  
}


// Calculates what integer level to set as the output based on the ratio between the desired voltage to get from the DAC and the DAC's
// reference voltage, taking a ratio of the two floating point numbers and multiplying by the constant number of bits the DAC can handle
short calcOutput(double voltage, double reference, bool bipolar){

  double fraction;

  // Offsets the fraction based on if the DAC mode is bipolar or not
  if (bipolar){
    fraction = (voltage + reference) / (2 * reference);
  }
  else{
    fraction = voltage / reference;    
  }
  
  short shortboi = short(fraction * pow(2,BITS)); 
  shortboi = shortboi << (MAX_BITS - BITS);         // Bit shifts appropriate amount to account for dummy bits
  return shortboi;
}


// sends 24-bit sequence to the DAC
void sendData(char header, short data, SPISettings settings){
  SPI.beginTransaction(settings);
  digitalWrite(SS, LOW);
  SPI.transfer(header);
  SPI.transfer16(data);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(30);
}

// returns an 8 bit header to send to the DAC before the data
char headerConstructor(char readWrite, char dacRegister, char channel){
  char header;
  
  header = readWrite << 7;      // RW logic bit
  header += 0 << 6;             // Reserved 0
  header += dacRegister << 3;   // Register for what you are sending data to
  header += channel;            // Which channel of the register are you controlling

  return header;
}


