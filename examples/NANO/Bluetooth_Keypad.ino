 

// Follower ADDR: 0021,13,0092DC
// Master ADDR: 0021,13,0062D6

#include "src/BluetoothModule.h"
#include <SoftwareSerial.h>
// Connect the HC-05 TX to Arduino pin 2 RX. 
// Connect the HC-05 RX to Arduino pin 3 TX through a voltage divider.

// Enter AT Mode: cycle power to BT device while holding button - should blink LED slowly

#define RX_PIN 6
#define TX_PIN 7

#define EN_PIN 13
#define KEY_PIN 12

#define BT_BAUD 38400


BluetoothModule BTmodule = BluetoothModule(EN_PIN, KEY_PIN, BT_BAUD, RX_PIN, TX_PIN, HIGH);

// String otherBTAddr = "0021,13,0092DC";
// String selfBTAddr = "0021,13,0062D6";


void setup() 
{
  Serial.begin(9600);
  Serial.println("Arduino is ready");
  Serial.println("Remember to select Both NL & CR in the serial monitor");  
}

void loop()
{
  // Keep reading from HC-05 and send to Arduino Serial Monitor
  if (BTmodule.BTserialAvailable()){
    String str = BTmodule.read();
    
    DynamicJsonDocument rawJson = BTmodule.convertToJsonDoc(str);
    if (BTmodule.determinePacketType(rawJson) == BluetoothModule::packetType::AT){
      int n = BTmodule.retrieveATcommandsLength(rawJson);
      String ATcmds[n];
      bool resetArr[n];
      BTmodule.retrieveATcommands(rawJson, ATcmds, resetArr, n);
      for (int i=0; i<n; i++)
        Serial.println("AT Command Received: " + ATcmds[i]);
    } else {
      Serial.println("Data Received: " + BTmodule.retrievePacketData(rawJson));
    }
    
  }

  // Keep reading from Arduino Serial Monitor and send to HC-05
  if (Serial.available())
  {
    String str = readAllFromSerial();
    String response;
    bool success = BTmodule.executeSingleATcommand(str, &response);
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
