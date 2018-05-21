/* Serial Communication Through USB
 * Version: Alpha 0.4
 * Thomas Kaunzinger
 * May 9, 2018
 * 
 * My take on creating a sort of protocol for sending and recieving code through serial input and executing
 * code based on the received data.
 * 
 * Acknowledgements:
 * codementor.io for their excellent explanation on implementation of linked lists in C++
 * Matt Stancliff for his guide to writing clean, modern C code
 * Efstathios Chatzikyriakidis and contributors for QueueArray library so I don't have to use my own
 */

///////////////
// LIBRARIES //
///////////////

#include <stdint.h>     // Because I was an idiot and thought that it was a good idea to use char to represent 8 bit ints
#include <QueueArray.h> // Because someone can do it better than I can so here it is

///////////////////////////////////
// CONSTANTS AND INITIALIZATIONS //
///////////////////////////////////

// Initializes pin 13 because it's the one that also controlls the on-board LED, so I can see the code executing
// without needing to use any actual hardware attached to any pinouts
const int_fast8_t LED_PIN = 13;

// Constant bytes that represent the characters being sent as commands
const int8_t READ = 'r';
const int8_t WRITE = 'w';
const int8_t DAC_A = 'a';
const int8_t DAC_B = 'b';
const int8_t DAC_2 = '2';
const int8_t DONE = '!';


// DAC CONTROL CONSTANTS //

// R/W (write is active low)
const int8_t READ_BIN = 1;
const int8_t WRITE_BIN = 0;

// Registers for controlling DAC
const int8_t DAC_REGISTER_BIN = 0;  // 000

// DAC channel addresses
const int8_t DAC_A_BIN = 0;   // 000
const int8_t DAC_B_BIN = 2;   // 010
const int8_t DAC_2_BIN = 4;   // 100

///////////
// SETUP //
///////////

void setup() {
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.begin(9600);
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
// HELPER FUNCTIONS //
//////////////////////

// I'll make this do something. It has to keep running through from the head to the tail ('!'), running a pre-set command based
// on each byte that is in this linked list buffer
void executeCommand(QueueArray <int8_t> &command){
  int8_t header;  // Initializes the data to send
  int16_t data;

  // Appends the read or write bits to the front of the header. Purges and ignores with with invalid syntax.
  if      (command.front() == READ)   {header += READ_BIN<<7;}
  else if (command.front() == WRITE)  {header += WRITE_BIN<<7;}
  else{
    purge(command);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    return;
  }
  
  command.pop();  // Clears out the already executed piece of the command

  header += 0 << 6;                 // Appends the reserved 0 bit
  header += DAC_REGISTER_BIN << 3;  // Appends the register for the DAC since I don't want the user to access anything else

  // Appends the DAC address to the header. Purges and ignores with with invalid syntax.
  if      (command.front() == DAC_A)  {header += DAC_A_BIN;}
  else if (command.front() == DAC_B)  {header += DAC_B_BIN;}
  else if (command.front() == DAC_2)  {header += DAC_2_BIN;}
  else{
    purge(command);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    return;
  }

  command.pop();  // Clears out the already executed piece of the command

  data += command.pop() << 8;   // First 8 bits of the data
  data += command.pop();        // Rest of the 8 bits of the data
  purge(command);               // Purges whatever is left of the command

  sendBits(header, data);   // Function to send the data to the DAC. Only reaches here if the whole command is valid.
  
}

// Empties a queue by popping everything in it. There's probably a faster way to do this.
void purge(QueueArray <int8_t> &queue){
  while(!queue.isEmpty()) queue.pop();
}


// I think I should start implementing this in the actual DAC controller program soon
void sendBits(int8_t header, int16_t data){

  // idk it's something dumb to make sure it actually worked
  for(int_fast8_t i = 0; i < 5; i++){
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}


