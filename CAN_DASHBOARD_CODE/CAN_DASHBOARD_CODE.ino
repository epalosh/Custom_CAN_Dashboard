///////////////////////////////////////////////////////////////////
// Code by Ethan Palosh, for USC Racing (Formula SAE).
///////////////////////////////////////////////////////////////////

// This project is a culmination of many months of work spent researching, prototyping and manufacturing circuits, and writing code.

// Hardware used in this project:
//    Arduino R4 Wifi
//    MCP2515 CAN Controller
//    MCP2551 CAN Tranceiver
//    CrystalFontz CFAG24064A-YYH-TZ 240x64 LCD Screen

// Libraries used in this code:
//    Screen: u8g2                          (https://github.com/olikraus/u8g2)
//    CAN: mcp_can                          (https://github.com/coryjfowler/MCP_CAN_lib)

///////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////

#include "Arduino.h" 
#include "math.h"
#include "stdlib.h"
#include "mcp_can.h"                        // CAN Library
#include "U8g2lib.h"                        // LCD screen library

// LCD Initialization.
U8G2_T6963_240X64_F_8080 u8g2(U8G2_R2, 4, 5, A4, A5, A0, A3, A2, A1, 7, 6, 1); //[full framebuffer, size = 1920 bytes]
#define CAN0_INT 2                          // Set INT to pin 2
MCP_CAN CAN0(10);                           // Set CS to pin 10

// Variables for dynamic calculations
int voltsInt = 0;                           // Integer portion of the number
int voltsDec = 0;                           // Decimal portion of the number
int rpmBarWidth = 0;                        // RPM bar value

// Cstring variables for output
char rpmStr[11];                            // RPM number for display
char batteryVoltageStr[16];                 // Battery voltage for display
char coolantFStr[15];                       // Coolant temp (F) for display
char oilPressureStr[11];                    // Oil pressure for display (psi)

// Stopwatch variables
char timer[18];                             // Timer string for display
unsigned long previousMillis = 0;           // Stores previous (standard time value)
const long interval = 1000;                 // Interval at which the stopwatch increments (ms)
unsigned long elapsedTime = 0;              // Total elapsed (standard time value)
int elapsedSeconds = 0;                     // Total elapsed seconds
int elapsedMinutes = 0;                     // Total elapsed minutes

// Screen update loop variables
unsigned long picturePreviousMillis = 0;    // Stores previous (standard time value)
const long pictureInterval = 150;           // Interval at which the screen updates (ms)

// Variables to track the following values NUMERICALLY:
int coolantC;                               // Coolant temp (C)
int coolantF;                               // Coolant temp (F)
int rpm;                                    // Engine rpm (rpm)
double volts;                               // Battery voltage (volts)
int oilPressure;                            // Engine oil pressure (psi)

// Variables to track the following values after converting from hex. In STRING form.
String engineRPM = "-";
String coolantTemp = "-";
String oilPressureCAN = "-";
String batteryVolts = "-";

// CAN variables
long unsigned int rxId;                     // Received message CAN ID
unsigned char len = 0;                      // Length of received message
unsigned char rxBuf[8];                     // Buffer to store received bits
char msgString[128];                        // Array to store serial string (print statements)

/////////////////////////////////////////////////////
//////////////////// Begin setup ////////////////////
void setup()
{
  // Begin terminal
  Serial.begin(115200);

  // Screen setup
  u8g2.begin();

  // Initializing numeric variables
  coolantC = 0;
  rpm = 0;
  coolantF = 0;
  volts = 0;
  oilPressure = 0;

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");
  
  CAN0.setMode(MCP_NORMAL);                             // Set operation mode to normal so the MCP2515 sends acks to received data.
  pinMode(CAN0_INT, INPUT);                             // Configuring pin for /INT input
  Serial.println("MCP2515 Library Receive Example...");
}
//\\\\\\\\\\\\\\\\\\   End setup   \\\\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

/////////////////////////////////////////////////////////
//////////////////// Begin main loop ////////////////////
void loop()
{
  if(!digitalRead(CAN0_INT))                            // If CAN0_INT pin is low, read receive buffer
  {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);                // Read data: len = data length, buf = data byte(s)
    
    // Determine if ID is standard (11 bits) or extended (29 bits)
    if((rxId & 0x80000000) == 0x80000000) {
      // Extended
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len); 
    } else {
      // Standard
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

      // Filtering messages by CAN ID and extracting values:
      if(rxId == 0x640){                                                      // 0x640: RPM is the first 2 bytes

        engineRPM = convertBufferRangeToDecimalString(rxBuf, 1, 0, 0, 1);              
        //Serial.println("--- updating rpm ---");
        rpm = (int)engineRPM.toFloat();

      } else if (rxId == 0x644) {                                             // 0x644: Engine oil pressure is the last 2 bytes

        oilPressureCAN = convertBufferRangeToDecimalString(rxBuf, .01, 0, 6, 7);    // Scale 1/100
        //Serial.println("--- updating oil pressure ---");
        oilPressure = (int)oilPressureCAN.toFloat();
        
      } else if (rxId == 0x649) {                                             // 0x649: Coolant temp is the 2nd byte, Battery voltage is 6th byte

        coolantTemp = convertBufferRangeToDecimalString(rxBuf, 1, -40, 0, 0);        // Offset -40
        //Serial.println("--- updating coolant temp ---");
        coolantC = (int)coolantTemp.toFloat();
        coolantF = (int)(coolantC * 9.0 / 5.0 + 32.0); // Calculating + updating coolant temp in fahrenheit

        batteryVolts = convertBufferRangeToDecimalString(rxBuf, .1, 0, 5, 5);
        //Serial.println("--- updating battery voltage ---");
        volts = batteryVolts.toFloat();

      }
    }
  
    //Serial.print(msgString);
  
    if((rxId & 0x40000000) == 0x40000000){    // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      //Serial.print(msgString);
    } else {
      for(byte i = 0; i<len; i++){
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        //Serial.print(msgString);
      }
    }
        
    //Serial.println();
  }

  // Print incoming values to terminal, for debug purposes:
  //Serial.println("rpm: " + String(engineRPM) + " | coolantTemp: " + String(coolantTemp) + " | oilPressure: " + String(oilPressureCAN) + " | Battery: " + String(volts));

  // Stopwatch (time running) logic - based on arduino internal timer.
  unsigned long currentMillis = millis();                                   // Time to update screen (100ms)
  if (currentMillis - picturePreviousMillis >= pictureInterval) {
    picturePreviousMillis = currentMillis;
    
    // Picture loop
    u8g2.firstPage();  
    do {
      draw();
    } while( u8g2.nextPage() );

    unsigned long clockCurrentMillis = millis();                            // Time to update timer (1000ms)
    if (clockCurrentMillis - previousMillis >= interval) {
      previousMillis = clockCurrentMillis;
      elapsedTime += 1000;
      elapsedSeconds++;
      if (elapsedSeconds == 60) {
        elapsedSeconds = 0;
        elapsedMinutes++;
      }
    }
  }
}
//\\\\\\\\\\\\\\\\\\ End of main loop \\\\\\\\\\\\\\\\\\\\
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

//////////////////////////////////////////////////////////
//////////////////// Helper functions ////////////////////

// Primary draw function (called every 100 ms in main loop)
void draw(void) {
  u8g2.setFont(u8g2_font_8x13B_tr);

  // Dynamic math to display values
  voltsInt = (int)volts/1;
  voltsDec = (int)((volts - (int)volts) * 100);
  rpmBarWidth = rpm/54;

  // Formatting dynamic variables
  snprintf (rpmStr, 11, "RPM: %d", rpm);
  snprintf (batteryVoltageStr, 16, "Battery: %0d.%dv", voltsInt, voltsDec);
  snprintf (coolantFStr, 15, "Coolant: %0dF", coolantF);
  snprintf (oilPressureStr, 11, "Oil: %0dpsi", oilPressure);

  snprintf (timer, 16, "Running: %02d:%02ds", elapsedMinutes, elapsedSeconds);

  // Drawing the variables
  //u8g.drawStr(168, 32, rpmStr); //align rpm right
  u8g2.drawStr(84, 32, rpmStr); //align rpm center
  u8g2.drawStr(121, 62, batteryVoltageStr);
  u8g2.drawStr(121, 50, timer);
  u8g2.drawStr(0, 51, oilPressureStr);
  u8g2.drawStr(0, 64, coolantFStr);

  // Drawing rpm bar
  u8g2.drawRBox(0, 2, rpmBarWidth, 16, 0); //dynamic

  // Drawing static decorators
  u8g2.drawRFrame(0, 0, 240, 20, 0);
  u8g2.drawLine(0, 37, 240, 37);

}

// Function to: 
//  CONVERT (a range of bytes from a HEX buffer) -> (string representation of its decimal values)
//  MULTIPLY by a scaler (many numbers come in x100, etc. to send decimal precsion)
//  ADD an offset if needed (many numbers come in at an offset, +40, etc. to send negative values)
String convertBufferRangeToDecimalString(const unsigned char *buffer, double scalerMultiple, int additiveOffset, size_t start, size_t end) {
  // Calculate the number of bytes to process
  size_t length = end - start + 1;
  
  // Ensure length does not exceed the maximum for a uint32_t (4 bytes)
  if (length > 4) {
    Serial.println("Range too large to fit in a 32-bit integer.");
    return "0";
  }

  uint32_t result = 0;

  // Convert the specified bytes in the range to a single integer
  for (size_t i = start; i <= end; i++) {
    result = (result << 8) | static_cast<uint8_t>(buffer[i]);
  }

  // Scale the result as a double
  double scaledResult = result * scalerMultiple + additiveOffset;

  // Convert the resulting double to a decimal string with desired precision
  char resultString[20]; // 20 chars to accommodate double representation
  sprintf(resultString, "%.2f", scaledResult); // Adjust precision as needed

  return String(resultString);
}
