/* DAC Controller
 * Version 0.3
 * 
 * Thomas Kaunzinger
 * Xcerra Corp.
 * April 11, 2018
 * 
 * This program is designed to select and output a desired voltage from an AD5722/AD5732/AD5752 DAC using SPI
 * http://www.analog.com/media/en/technical-documentation/data-sheets/AD5722_5732_5752.pdf
 */

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

////////////////////
// USER VARIABLES //
////////////////////

// Set REFERENCE_VOLTAGE to the voltage of the reference pin
const double REFERENCE_VOLTAGE = 2.048;

// Set DESIRED_VOLTAGE to the voltage you wish to produce from the DAC
const double DESIRED_VOLTAGE_1 = 1.024;
const double DESIRED_VOLTAGE_2 = 0.512;

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
const int BITS = 14;              // Number of Bits of precision
const bool BIPOLAR = true;        // Currently, the DAC is set to have a bipolar output
const int CLOCK_SPEED = 5000000;  // DAC is rated for 30MHz, Arduino clock is much lower?

// SPI settings
// This uses SPI_MODE1 or SPI_MODE2 but I'm not 100% sure which one it is. I think 2 but I can't tell.
SPISettings DEFAULT_SETTINGS(CLOCK_SPEED, MSBFIRST, SPI_MODE2);


//////////////////////////////////////////////////////////////////////

///////////
// SETUP //
///////////
void setup() {

  // Set up slave-select pin. SPI lib handles others
  digitalWrite(SS, HIGH);

  // Sets LDAC to low as seen on data sheet
  pinMode(LDAC, OUTPUT);
  digitalWrite(LDAC, LOW);

  // Initializes the SPI protocol
  SPI.begin();

}


//////////////////////////////////////////////////////////////////////

////////////////////
// EXECUTION LOOP //
////////////////////
void loop() {
  
  setOutput(DESIRED_VOLTAGE_1, DESIRED_VOLTAGE_2, REFERENCE_VOLTAGE, DEFAULT_SETTINGS, BIPOLAR);
  delay(1000);

}


//////////////////////////////////////////////////////////////////////

//////////////////////
// HELPER FUNCTIONS //
//////////////////////

// Enables the slave select for the DAC, calculates the value for the desired voltage, and communicates to DAC through SPI before closing the SS
void setOutput(double desired1, double desired2, double reference, SPISettings settings, bool bipolar){

  // header to address that you are programming to the DAC
  char readWrite = 0;     // Write is active low bit
  char reserved = 0;      // Resserved 0 bit
  char dacRegister = 0;   // 3 bits, indicates accessing of DAC register
  char dacChannel;        // initializes the variable to address one or both of the DACs

  // Creates an 8-bit header to send to the chip to show where it is writing its information to
  char header;
  header = readWrite << 7;
  header += reserved << 6;
  header += dacRegister << 3;

  // checks if the two variables are exactly the same (e.g. calling setOutput(DESIRED_VOLTAGE_1, DESIRED_VOLTAGE_1, ...)) and uses the 'Both' address
  if (desired1 == desired2){

    // 100 writes to both DACs
    dacChannel = 4;
    header += dacChannel;
    
    SPI.beginTransaction(settings);
    digitalWrite(SS, LOW);
    SPI.transfer(header);   // transfers 8 bit char header
    SPI.transfer16(calcOutput(desired1, reference, bipolar));  // transfers 16 bits of data (including dummy bits at end)
    digitalWrite(SS, HIGH);
    SPI.endTransaction();
    
  }

  // Sets DAC A and then DAC B to the two desired voltages
  else{

    // 000 writes to DAC A
    dacChannel = 0;
    header += dacChannel;
    
    SPI.beginTransaction(settings);
    digitalWrite(SS, LOW);
    SPI.transfer(header);   // transfers 8 bit char header
    SPI.transfer16(calcOutput(desired1, reference, bipolar));  // transfers 16 bits of data (including dummy bits at end)
    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    // Removes the address from the first DAC
    header -= dacChannel;


    // 010 writes to DAC B
    dacChannel = 2;
    header += dacChannel;

    SPI.beginTransaction(settings);
    digitalWrite(SS, LOW);
    SPI.transfer(header);   // transfers 8 bit char header
    SPI.transfer16(calcOutput(desired2, reference, bipolar));  // transfers 16 bits of data (including dummy bits at end)
    digitalWrite(SS, HIGH);
    SPI.endTransaction();
    
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




