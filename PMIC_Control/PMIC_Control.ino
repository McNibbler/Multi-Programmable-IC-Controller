#include <Wire.h>
#include "LT_PMBusMath.h"


const int ledPin = 13;


void setup() {
  Wire.begin(); // Initiate the Wire library
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Temperature Analysis: Returns internal temperature in degrees C");
  Serial.println("-----------------------------------------------------------------------");
  delay(100);
}

void loop() {
  readReg((uint8_t) 0x5A, (uint8_t) 0x8E);
//  delay(500);
//  writeReg((uint8_t) 0x5A, (uint8_t) 0x50, (uint8_t) 0x80);
//  delay(500);
//  readReg((uint8_t) 0x5A, (uint8_t) 0x50);
//  delay(500);
//  writeReg((uint8_t) 0x5A, (uint8_t) 0x50, (uint8_t) 0xB8);
//  delay(500);
//  readReg((uint8_t) 0x5A, (uint8_t) 0x50);
  delay(1500);
}

/**
 * Serial output of register value of a 
 */
void readReg(uint8_t deviceAddress, uint8_t regAddress) {
  Wire.beginTransmission( deviceAddress );
  Wire.write( regAddress );
  Wire.endTransmission( false );
  
  Wire.requestFrom((uint8_t) deviceAddress, (uint8_t) 2 , (uint8_t) true );
  
  if ( Wire.available() >= 2 )
  {
    uint8_t result[2];
//    uint8_t result = Wire.read();
//    uint8_t result2 = Wire.read();
    Wire.readBytes(result, 2);
    unsigned int total_result = ((unsigned int) result[1] << 8) | ((unsigned int) result[0] & 0xFF);
    
    if ( regAddress == (uint8_t) 0x8E) {
      Serial.println(math_.lin11_to_float(total_result));
      /*Serial.print("Temperature reading of : ");
      Serial.print(math_.lin11_to_float(total_result));
      Serial.print(" (lin11 to float) \t");
      Serial.print(result[1], BIN);
      Serial.print(result[0], BIN);
      Serial.print("\t");
      Serial.print(total_result, BIN);
      Serial.print(" (regular) ");
      Serial.print(" C from register ");
      Serial.println( regAddress, HEX );*/
    }
    else {
      Serial.print( "Value of : " );
      Serial.print( result[0], DEC );
      Serial.print(" reading off of register: " );
      Serial.println( regAddress, HEX );
    }
  } 
}

void writeReg(uint8_t deviceAddress, uint8_t regAddress, uint8_t value) {
  Serial.print("Writing " );
  Serial.print(value);
  Serial.print(" to register ");
  Serial.println(regAddress, HEX);
  Wire.beginTransmission( deviceAddress );
  Wire.write( regAddress );
  Wire.write( value );
  Wire.endTransmission(  ); //Note: TRUE for this releases the i2c bus. unsure if false yields asynchronous reads? 
  alert(100);
}

void alert(int x) {
  digitalWrite(ledPin, HIGH);
  delay(x);
  digitalWrite(ledPin, LOW); 
  delay(x); 
  digitalWrite(ledPin, HIGH);
  delay(x);
  digitalWrite(ledPin, LOW); 
}

