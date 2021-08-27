 

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

BluetoothModule BTmodule(EN_PIN, KEY_PIN, BT_BAUD, RX_PIN, TX_PIN);

String otherBTAddr = "0021,13,0092DC";
String selfBTAddr = "0021,13,0062D6";

String otherConfig[] = {
  "AT+ORGL",
  "AT+ROLE=1",
  "AT+UART=38400,0,0",
  "AT+IPSCAN=1024,1,1024,1",
  "AT+BIND=" + selfBTAddr
};
bool otherConfigResetArr[] = {
  true,
  true,
  false,
  false,
  false,
};
int sendArrSize = sizeof(otherConfig) / sizeof(otherConfig[0]);

String selfConfigOnStart[] = {
  "AT+DISC",
  "AT+ROLE=1",
  "AT+UART=38400,0,0",
  "AT+BIND=" + otherBTAddr,
};
bool selfConfigOnStartResetArr[] = {
  false,
  true,
  false,
  false,
  false,
};
int selfOnStartArrSize = sizeof(selfConfigOnStart) / sizeof(selfConfigOnStart[0]);

String selfConfigLast[] = {
  "AT+ORGL",
  "AT+UART=38400,0,0",
  "AT+IPSCAN=1024,1,1024,1",
  "AT+RESET"
};
bool selfConfigLastResetArr[] = {
  true,
  false,
  false,
  false,
};

int selfLastArrSize = sizeof(selfConfigLastResetArr) / sizeof(selfConfigLastResetArr[0]);

 
void setup() 
{
    Serial.begin(9600);
    Serial.println("Arduino is ready");
    Serial.println("Remember to select Both NL & CR in the serial monitor");

    BTmodule.startup();

    BTmodule.executeATCommands(selfConfigOnStart, selfConfigOnStartResetArr, selfOnStartArrSize, BluetoothModule::ATmode::LIMITED);
    BTmodule.powerResetFlush();

    int timeout = 20;
    if (BTmodule.waitForConnection(timeout)){
      BTmodule.sendATCommands(otherConfig, otherConfigResetArr, sendArrSize);
      Serial.println("Commands Sent");

      BTmodule.executeATCommands(selfConfigLast, selfConfigLastResetArr, selfLastArrSize, BluetoothModule::ATmode::FULL);
      
    } else {
      Serial.println("Connection Timed-Out");
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
      Serial.println(response);
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