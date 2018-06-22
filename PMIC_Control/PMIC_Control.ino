#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <Wire.h>

#include <LT_SMBusPec.h>
#include <LT_SMBusNoPec.h>
#include <LT_SMBusGroup.h>
#include <LT_PMBus.h>
#include <LT_PMBusMath.h>
#include <LT_PMBusDevice.h>
#include <LT_PMBusRail.h>
#include <LT_PMBusDetect.h>
#include <LT_PMBusDetect.h>
#include <LT_PMBusSpeedTest.h>
#include <LT_PMBusDeviceLTC3880.h>
#include <LT_FaultLog.h>
#include <LT_3880FaultLog.h> // For compilation of library

/**
 * Control code for a PMIC (Power Management IC), specifically the LTC2977. 
 */

/**
 * PMIC programming explained:
 *  -How PMBus is built up:
 *    PMBus uses SMBus as a base, which uses I2C as a base. It's not super important to the specific details of this, 
 *    but just know that PMBus is an abstracted, enchanced version of I2C.
 *      The biggest utilization of this script is the use of "Paging" to monitor and configure multiple i/o lines
 *      for one device. The script uses PMBUS alongside Linear Tech provided commands to set output voltages,
 *      sense and monitor different voltages, and perform power sequencing tasks.
 * 
 *  
 */


const int ledPin = 13;

typedef struct paged_data {
  uint16_t val;
  uint8_t page;
  paged_data* next;
} paged_list;


#define VIN_ON                      0x35
#define VIN_OFF                     0x36
#define VOUT_OV_FAULT_LIMIT         0x40
#define VOUT_OV_FAULT_RESPONSE      0x41
#define VOUT_UV_FAULT_LIMIT         0x44
#define VIN_OV_WARN_LIMIT           0x57
#define VIN_UV_WARN_LIMIT           0x58
#define VIN_UV_FAULT_RESPONSE       0x5A
#define POWER_GOOD_ON               0x5E
#define POWER_GOOD_OFF              0x5F
#define TON_DELAY                   0x60
#define TON_RISE                    0x61
#define TON_MAX_FAULT_LIMIT         0x62
#define TON_MAX_FAULT_RESPONSE      0x63
#define TOFF_DELAY                  0x64
#define MFR_CONFIG_LTC2977          0xD0
#define MFR_CONFIG_ALL_LTC2977      0xD1
#define MFR_FAULT_B00_RESPONSE      0xD2
#define MFR_FAULT_B01_RESPONSE      0xD3
#define MFR_FAULT_B10_RESPONSE      0xD4
#define MFR_FAULT_B11_RESPONSE      0xD5
#define MFR_VINEN_OV_FAULT_RESPONSE 0xD9
#define MFR_VINEN_UV_FAULT_RESPONSE 0xDA


#define STATUS_BYTE 0x78
#define STATUS_WORD 0x79

#define LTC2977_I2C_ADDRESS 0x33
#define VOUT_MODE                   (smbus->readByte(LTC2977_I2C_ADDRESS, 0x20))

//Init smbus device detector without packet error checking
static LT_SMBusNoPec *smbus = new LT_SMBusNoPec(100000); //100kHZ I2C transfer speed
static LT_PMBus *pmbus = new LT_PMBus(smbus);
static uint8_t ltc2977_i2c_address;


/**
 * Setup controls the intialization of necessary libraries, instantiation of classes/structs as necessary, etc. 
 */
void setup() {
//  Wire.begin(); // Initiate the Wire library
  pinMode(ledPin, OUTPUT);
  //2 quick LED flashes to signify that the leds were set up properly.
  alert(200);
  Serial.begin(9600);

  ltc2977_i2c_address = LTC2977_I2C_ADDRESS;

  setupAllRegisters();
  // pmbus->sequenceOnGlobal(); 
  delay(200);
  // Serial.println(math_.lin11_to_float(smbus->readWord(ltc2977_i2c_address, 0x35)));
  // uint8_t fault_response = smbus->readByte(ltc2977_i2c_address, 0x5A);
  // fault_response = fault_response & ~(1<<7);
  // smbus->writeByte(ltc2977_i2c_address, 0x5A, fault_response);
  // Serial.println(smbus->readByte(ltc2977_i2c_address, 0x5A));
  // // pmbus->setPage(ltc2977_i2c_address, 0x00);
  // pmbus->setVoutWithPage(ltc2977_i2c_address, 3.0, 0x00);

  // pmbus->setPageWithPolling(ltc2977_i2c_address, 0x00);
  // // smbus->waitForAck(ltc2977_i2c_address, 0x00);
  // // smbus->writeByte(ltc2977_i2c_address, 0x01, 0b01000000);


  delay(100);
  Serial.println(math_.lin16_to_float(smbus->readWord(ltc2977_i2c_address, 0x25), smbus->readByte(ltc2977_i2c_address, 0x20) & 0x1F));
  
 // Serial.println("Temperature Analysis: Returns internal temperature in degrees C");
  // Serial.println("-----------------------------------------------------------------------");
  delay(100);
}


void loop() {
//need to monitor voltage sense for channels 6 and 7 - pages 0 and 1 for the ltc2977.
  float voltage;
  uint8_t faults;
  for (int i = 0; i < 8; i++){ 
    pmbus->setPage(ltc2977_i2c_address, i);
    // voltage = pmbus->readVout(ltc2977_i2c_address, false);
    // Serial.print(F("LTC2977 VOUT "));
    // Serial.print(i + " ");
    // Serial.println(voltage, DEC);
    delay(2000);
    faults = smbus->readByte(ltc2977_i2c_address, 0x7A);
    // Serial.print(F("Channel "));
    // Serial.print(i+1);
    // Serial.print(F(" Faults are "));
    // Serial.println(faults, BIN);
    pmbus->setPage(ltc2977_i2c_address, 0x00);
    uint16_t config = smbus->readWord(ltc2977_i2c_address, 0xD0);
    Serial.println(config, BIN);
    
    delay(200);
  }
}


LT_PMBusMath::lin11_t fl_to_lin11(float xin) {
  return math_.float_to_lin11(xin);
}

LT_PMBusMath::lin16_t fl_to_lin16(float xin) {
  return math_.float_to_lin16(xin, VOUT_MODE);
}

void setupAllRegisters() {
  paged_list* current_paged_data;
  smbus->writeWord(ltc2977_i2c_address, VIN_ON, fl_to_lin11(4.50));

  smbus->writeWord(ltc2977_i2c_address, VIN_OFF, fl_to_lin11(4.0));

  current_paged_data = get_vout_fault_limit_page();
  execute(false, VOUT_OV_FAULT_LIMIT, current_paged_data->val);
  free(current_paged_data);

  current_paged_data = get_vout_fault_response_page();
  execute(false, VOUT_FAULT_RESPONSE)
  free(current_paged_data);

  
}

void execute(bool isByte, uint8_t pmbus_command, paged_list* data) {
  while (data != NULL) {
    pmbus->setPage(ltc2977_i2c_address, data->page);
    if (isByte) {
      smbus->writeByte(ltc2977_i2c_address, pmbus_command, data->val);
    }
    else {
      smbus->writeWord(ltc2977_i2c_address, pmbus_command, data->val);
    }
    data = data->next;
  }
}

paged_list* get_vout_fault_limit_page() {
  // Init linked list
  paged_list* head = NULL;
  head = malloc(sizeof(paged_list));
  //lin16_t and lin11_t are typedef'd as uints so this is fine
  head->val = fl_to_lin16(2.1); 
  head->page = 0x00; 
  head->next = NULL; //only one output channel

  return head;
}

paged_list* get_vout_fault_response_page() {
  paged_list* head = NULL;
  head = malloc(sizeof(paged_list));
  //lin16_t and lin11_t are typedef'd as uints so this is fine
  head->val = 0b00000000;
  // b[7:6] - 00  -> unit continues without interruption
  // b[5:3] - 000 -> unit does not attempt to restart  
  // b[2:0] - 000 -> unit turns off immediately on seeing a fault
  head->page = 0x00; 
  head->next = NULL; //only one output channel

  return head;
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
