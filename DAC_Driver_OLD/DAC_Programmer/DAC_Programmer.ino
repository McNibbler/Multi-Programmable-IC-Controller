/* DAC Controller
 * Version: 1.1
 * 
 * Thomas Kaunzinger
 * Xcerra Corp.
 * May 4, 2018
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

///////////////
// LIBRARIES //
///////////////

#include <SPI.h>
#include <math.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

////////////////////
// USER VARIABLES //
////////////////////

// Set REFERENCE_VOLTAGE to the voltage of the reference pin
const double REFERENCE_VOLTAGE = 2.5;

// Set DESIRED_VOLTAGE to the voltage you wish to produce from the DAC
const double DESIRED_VOLTAGE_1 = -1.25;
const double DESIRED_VOLTAGE_2 = 0.75;

// Choose between bipolar and unipolar data
const bool BIPOLAR = true;

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
// This uses SPI_MODE1 or SPI_MODE2 but I'm not 100% sure which one it is. I think 2 but I can't tell.
SPISettings DEFAULT_SETTINGS(CLOCK_SPEED, MSBFIRST, SPI_MODE2);

// Input pins for DACs to read back voltages
// NOT TO BE USED IF BIPOLAR, ARDUINO ANALOG IN IS ONLY RATED FROM 0V TO 5V
const int_fast8_t DAC_1 = A4;
const int_fast8_t DAC_2 = A5;


///////////////////////////
// DAC CONTROL CONSTANTS //
///////////////////////////

// R/W (write is active low)
const int8_t READ = 1;
const int8_t WRITE = 0;

// Registers for controlling DAC
const int8_t DAC_REGISTER = 0;      // 000
const int8_t RANGE_REGISTER = 1;    // 001
const int8_t POWER_REGISTER = 2;    // 010
const int8_t CONTROL_REGISTER = 3;  // 011

// DAC channel addresses
const int8_t DAC_A = 0;     // 000
const int8_t DAC_B = 2;     // 010
const int8_t DAC_BOTH = 4;  // 100

// Control channel addresses
const int8_t NOP = 0;       // 000
const int8_t TOGGLES = 1;   // 001
const int8_t CLR = 4;       // 100
const int8_t LOAD = 5;      // 101

// Output range select register
const int8_t UNI_5 = 0;    // 000
const int8_t UNI_10 = 1;   // 001
const int8_t UNI_108 = 2;  // 010
const int8_t BI_5 = 3;     // 011
const int8_t BI_10 = 4;    // 100
const int8_t BI_108 = 5;   // 101

// DAC's gain
const double GAIN = 2;



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
  int8_t rangeHeaderA = headerConstructor(WRITE, RANGE_REGISTER, DAC_A);        // I have to write to all 3 of these channels individually.
  int8_t rangeHeaderB = headerConstructor(WRITE, RANGE_REGISTER, DAC_B);        // Writing to "BOTH" apparently isn't enough smh.
  int8_t rangeHeaderBoth = headerConstructor(WRITE, RANGE_REGISTER, DAC_BOTH);
  if (BIPOLAR){
    sendData(rangeHeaderA, BI_5, DEFAULT_SETTINGS);
    sendData(rangeHeaderB, BI_5, DEFAULT_SETTINGS);
    sendData(rangeHeaderBoth, BI_5, DEFAULT_SETTINGS);
  }
  else{
    sendData(rangeHeaderA, UNI_5, DEFAULT_SETTINGS);
    sendData(rangeHeaderB, UNI_5, DEFAULT_SETTINGS);
    sendData(rangeHeaderBoth, UNI_5, DEFAULT_SETTINGS);
  }


  // Sets up DAC preferences
  int8_t controlToggleHeader = headerConstructor(WRITE, CONTROL_REGISTER, TOGGLES);
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
  int8_t powerHeader = headerConstructor(WRITE, POWER_REGISTER, int16_t(0));
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
  int8_t loadHeader = headerConstructor(WRITE, CONTROL_REGISTER, LOAD);
  sendData(loadHeader, int16_t(0), DEFAULT_SETTINGS);


  // Slight delay before sending the data to the DAC repeatedly in the loop
  delay(1000);
}


//////////////////////////////////////////////////////////////////////

////////////////////
// EXECUTION LOOP //
////////////////////
void loop() {

  // Compensates for the gain of the DAC
  double voltageCompensated1 = DESIRED_VOLTAGE_1 / GAIN;
  double voltageCompensated2 = DESIRED_VOLTAGE_2 / GAIN;

  // Writes the desired outputs to the DACs
  setOutput(voltageCompensated1, voltageCompensated2, REFERENCE_VOLTAGE, DEFAULT_SETTINGS, BIPOLAR);
  
  // For loading the data
  int8_t loadHeader = headerConstructor(WRITE, CONTROL_REGISTER, LOAD);
  sendData(loadHeader, int16_t(0), DEFAULT_SETTINGS);

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
  int8_t header = headerConstructor(0, 0, 0);   // Write is active low, DAC register is 000, dacChannel is set to 0 to initialize

  // Calculates the bits of data to send to the DAC
  int16_t bits1 = calcOutput(desired1, reference, bipolar);
  int16_t bits2 = calcOutput(desired2, reference, bipolar);

  int8_t dacChannel;
  
  // checks if the two variables are exactly the same (e.g. calling setOutput(DESIRED_VOLTAGE_1, DESIRED_VOLTAGE_1, ...)) and uses the 'Both' address
  if (desired1 == desired2){
    
    header += DAC_BOTH;
    sendData(header, bits1, settings);
    
  }


  // Sets DAC A and then DAC B to the two desired voltages
  else{
    
    header += DAC_A;
    sendData(header, bits1, settings);
    header -= DAC_A; // Removes the address from the first DAC (yes I know it's already 0 but like just to be safe)

    header += DAC_B;
    sendData(header, bits2, settings);
    
  }
  
}


// Calculates what integer level to set as the output based on the ratio between the desired voltage to get from the DAC and the DAC's
// reference voltage, taking a ratio of the two floating point numbers and multiplying by the constant number of bits the DAC can handle
int16_t calcOutput(double voltage, double reference, bool bipolar){
  double fraction;

  // Offsets the fraction based on if the DAC mode is bipolar or not
  if (bipolar){
    fraction = (voltage + reference) / (2 * reference);
  }
  else{
    fraction = voltage / reference;    
  }
  
  int16_t shortboi = int16_t(fraction * pow(2,BITS)); 
  shortboi = shortboi << (MAX_BITS - BITS);         // Bit shifts appropriate amount to account for dummy bits
  
  return shortboi;
}


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
