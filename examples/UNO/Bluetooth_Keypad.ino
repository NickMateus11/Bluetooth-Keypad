 

// Follower ADDR: 0021,13,0092DC
// Master ADDR: 0021,13,0062D6

#include "src/BluetoothModule.h"
#include <SoftwareSerial.h>
// Connect the HC-05 TX to Arduino pin 2 RX. 
// Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.

// Enter AT Mode: cycle power to BT device while holding button - should blink LED slowly

#define RX_PIN 2
#define TX_PIN 3

#define EN_PIN 5
#define KEY_PIN 4

#define BT_BAUD 38400


BluetoothModule BTmodule = BluetoothModule(EN_PIN, KEY_PIN, BT_BAUD, RX_PIN, TX_PIN, HIGH);

// String otherBTAddr = "0021,13,0092DC";
// String selfBTAddr = "0021,13,0062D6";

String selfConfigOnStart[] = {
  "AT+DISC",
  "AT+UART=38400,0,0",
  "AT+RESET"
};
bool selfConfigOnStartResetArr[] = {
  true,
  false,
  false
};
int ATmodeType = BluetoothModule::ATmode::LIMITED;
int selfOnStartArrSize = sizeof(selfConfigOnStart) / sizeof(selfConfigOnStart[0]);

 
void setup() 
{
  Serial.begin(9600);
  Serial.println("Arduino is ready");
  Serial.println("Remember to select Both NL & CR in the serial monitor");

  int timeout = 20;
  if (BTmodule.waitForConnection(timeout)){
    Serial.println("Connection Verified");
  } else {
    Serial.println("Connection Timed-Out");
  }

  BTmodule.sendATCommands(selfConfigOnStart, selfConfigOnStartResetArr, selfOnStartArrSize);
  BTmodule.send("Hello - DATA");
  
}

void loop()
{
  // Keep reading from HC-05 and send to Arduino Serial Monitor
  if (BTmodule.BTserialAvailable()){
    String str = BTmodule.read();
    Serial.println(str);
  }

  // Keep reading from Arduino Serial Monitor and send to HC-05
  if (Serial.available())
  {
    String str = readAllFromSerial();
    String response;
    bool success = BTmodule.executeSingleATcommand(str, &response);
    if (!success) Serial.println("ERROR: " + response);
  }
 
}

String readAllFromSerial(){
  char buff[256];
  int idx=0;
  while (Serial.available()){
    buff[idx] = Serial.read();
    if (buff[idx] != '\r' and buff[idx] != '\n') idx++;
    delay(2);
  }
  buff[idx] = '\0';
  return String(buff);
}
