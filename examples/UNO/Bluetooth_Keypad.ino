 

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

String otherBTAddr = "0021,13,0092DC";
String selfBTAddr = "0021,13,0062D6";

const char* sendConfig[] = {
  "AT+ORGL",
  "AT+ROLE=1",
  "AT+UART=38400,0,0",
  "AT+IPSCAN=1024,1,1024,1",
  ("AT+BIND=0021,13,0062D6" + selfBTAddr).c_str()
};
bool sendConfigFuncs[] = {
  true,
  true,
  false,
  false,
  false
};
int sendArrSize = sizeof(sendConfig) / sizeof(sendConfig[0]);

const char* selfConfig1[] = {
   "AT+DISC",
   "AT+ROLE=1",
   "AT+UART=38400,0,0",
   ("AT+BIND=0021,13,0062D6" + otherBTAddr).c_str()
 };
bool selfConfigFuncs1[] = {
  false,
  true,
  false,
  false,
  false
};
int selfArrSize1 = sizeof(selfConfig1) / sizeof(selfConfig1[0]);

const char* selfConfig2[] = {
  "AT+ORGL",
  "AT+UART=38400,0,0",
};
bool selfConfigFuncs2[] = {
  true,
  false,
};
int selfArrSize2 = sizeof(selfConfig2) / sizeof(selfConfig2[0]);


void setup() 
{
  Serial.begin(9600);
  Serial.println("Arduino is ready");
  Serial.println("Remember to select Both NL & CR in the serial monitor");

  BTmodule.executeATCommands(selfConfig1, selfConfigFuncs1, selfArrSize1, BluetoothModule::ATmode::LIMITED);
  if(BTmodule.waitForConnection(20, otherBTAddr)){
    if (BTmodule.sendATCommands(sendConfig, sendConfigFuncs, sendArrSize) && BTmodule.waitForACK()){
      BTmodule.executeATCommands(selfConfig2, selfConfigFuncs2, selfArrSize2, BluetoothModule::ATmode::FULL);
    }
  }
  
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
