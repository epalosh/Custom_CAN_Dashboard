///////////////////////////////////////////////////////////////////
// Code by Ethan Palosh, for USC Racing (Formula SAE).
///////////////////////////////////////////////////////////////////
//
// Modified "CAN Send Example" from https://github.com/coryjfowler/MCP_CAN_lib
//
// Values generated:
//    RPM
//    Oil pressure
//    Coolant temperature
//    Battery Voltage

#include <mcp_can.h>
#include <SPI.h>

MCP_CAN CAN0(10);     // Set CS to pin 10

void setup()
{
  Serial.begin(115200);

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) Serial.println("MCP2515 Initialized Successfully!");
  else Serial.println("Error Initializing MCP2515...");

  CAN0.setMode(MCP_NORMAL);   // Change to normal mode to allow messages to be transmitted
  srand(analogRead(0)); // Seed random generator for varied results
}


#include <stdlib.h>

byte rpm[8] = {0x21, 0x53, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
byte oilPressure[8] = {0x01, 0x11, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08};
byte coolantTemp[8] = {0x51, 0x2F, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

void loop()
{
  // Generate a random RPM value between 8000 and 9000
  int randomRPM = 8000 + rand() % 1001;
  rpm[0] = (randomRPM >> 8) & 0xFF; // Most significant byte
  rpm[1] = randomRPM & 0xFF;        // Least significant byte

  // Generate a random Oil Pressure value between 30 and 40 and assign it to 7th and 8th bytes
  int randomOilPressure = 3000 + rand() % 1100;
  oilPressure[6] = (randomOilPressure >> 8) & 0xFF; // 7th byte (index 6)
  oilPressure[7] = randomOilPressure & 0xFF;        // 8th byte (index 7)

  // Generate a random Coolant Temperature value between 180 and 200 and assign it to the first byte
  int randomCoolantTemp = 90 + rand() % 15;
  coolantTemp[0] = (randomCoolantTemp >> 8) & 0xFF; // First byte (index 0)
  coolantTemp[1] = randomCoolantTemp & 0xFF;        // Second byte (index 1)

  // Generate a random value for byte 6 of 0x649 (coolantTemp) between 11.0 and 13.0
  float randomCoolantByte6 = 110 + rand() % 31;  
  coolantTemp[5] = randomCoolantByte6;  // Scale to integer and store in 6th byte (index 5)

  // Send the CAN messages
  byte sndStat = CAN0.sendMsgBuf(0x640, 0, 8, rpm);
  byte sndStat2 = CAN0.sendMsgBuf(0x644, 0, 8, oilPressure);
  byte sndStat3 = CAN0.sendMsgBuf(0x649, 0, 8, coolantTemp);

  // Print the status of each message
  if(sndStat == CAN_OK){
    Serial.println("RPM Message Sent Successfully!");
  } else {
    Serial.println("Error Sending RPM Message...");
  }
  if(sndStat2 == CAN_OK){
    Serial.println("Oil Pressure Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Oil Pressure Message...");
  }
  if(sndStat3 == CAN_OK){
    Serial.println("Coolant Temp Message Sent Successfully!");
  } else {
    Serial.println("Error Sending Coolant Temp Message...");
  }

  delay(10);   // send data per 100ms
}
