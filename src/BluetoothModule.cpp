
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "BluetoothModule.h"

#define BT_BOOT_WAIT_TIME           1000
#define READ_EXTRA_WAIT_TIME        50
#define SEND_WAIT_TIME              READ_EXTRA_WAIT_TIME*2
#define SEND_BUFFER_PAUSE_WAIT_TIME READ_EXTRA_WAIT_TIME/2
#define BT_PIN_CHANGE_WAIT          10
#define SERIAL_BUFFER_LENGTH        64


const char* basicConfig[] = {
  "AT+DISC",
  "AT+ORGL",
  "AT+UART=38400,0,0",
  "AT+IPSCAN=1024,1,1024,1"
};
bool basicConfigResetArr[] = {
  true,
  true,
  false,
  false,
};
int basicConfigArrSize = sizeof(basicConfig) / sizeof(basicConfig[0]);


BluetoothModule::BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, uint8_t rxPin, uint8_t txPin, bool isOn){
    _enPin = enPin;
    _keyPin = keyPin;
    _baudRate = baudRate;
    
    _BTserial = new SoftwareSerial(rxPin, txPin);
    _BTserial->begin(_baudRate);

    pinMode(_enPin, OUTPUT);
    _setEnablePin(isOn);
    pinMode(_keyPin, OUTPUT);
    _setKeyPin(LOW);
}

void BluetoothModule::startup(){
  _setKeyPin(LOW);
  _setEnablePin(HIGH);

  Serial.println("-- Startup Complete --");
}

void BluetoothModule::read(char* buff){
  // char buff[256];
  int idx=0;
  bool courtesyWait=true;

  Serial.println("Reading");
  while (true){
    // read BT buffer and store char
    if (BTserialAvailable()){
      buff[idx++] = _BTserial->read();
      courtesyWait=true;
    }
    // allow time for more incoming data
    else if(courtesyWait){
      delay(READ_EXTRA_WAIT_TIME);
      courtesyWait=false;
    } 
    else break;
  }
  // remove trailing carriage and newline
  while(idx>0 && (buff[idx-1] == '\r' || buff[idx-1] == '\n')) idx--;
  
  // end string with null character
  buff[idx] = 0;

  Serial.print("Read Results: ");
  Serial.println(buff);  
}

bool BluetoothModule::waitForATandExecute(){
  flush();

  Serial.println("Waiting for AT command JSON data");  
  char incomingStr[256];
  while(!strlen(incomingStr) || (incomingStr[0]!='{' && incomingStr[strlen(incomingStr)-1]!='}')){
    read(incomingStr);
  }

  DynamicJsonDocument rawJson(256);
  deserializeJson(rawJson, incomingStr);

  if (determinePacketType(rawJson) != BluetoothModule::packetType::AT) return false;

  // Return acknowledgment of AT commands receieved
  sendData("ACK");

  Serial.println("Decoding AT commands from JSON");
  int n = retrieveATcommandsLength(rawJson);
  char* ATcmds[n];
  bool resetArr[n];
  retrieveATcommands(rawJson, ATcmds, resetArr, n);
  return executeATCommands((const char **)ATcmds, resetArr, n, ATmode::FULL);
}

void BluetoothModule::enterATmode(int setATmode){
  // power cycle and enable AT mode
  if (setATmode==ATmode::FULL) {
    Serial.println("Entering Full AT mode");
    _setEnablePin(LOW);
    _setKeyPin(HIGH);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);
  // limited AT mode - maintain connection
  } else if (setATmode==ATmode::LIMITED) {
    Serial.println("Entering LIMITED AT mode");
    _setKeyPin(HIGH);
    delay(100);
  } else return;

  _currATmode = static_cast<ATmode>(setATmode);

  flush();
}

void BluetoothModule::exitATmode(){
  // disable current AT mode - either full or limited
  if (_currATmode == ATmode::FULL) {
    Serial.println("Exiting Full AT mode");
    _setEnablePin(LOW);
    _setKeyPin(LOW);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);
  } else if (_currATmode == ATmode::LIMITED){
    Serial.println("Exiting LIMITED AT mode");
    _setKeyPin(LOW);
    delay(100);
  }

  _currATmode = ATmode::OFF;

  // flush();
}

void BluetoothModule::powerReset(){
  Serial.println("Power Resetting");
  // cycle enable pin
  _setEnablePin(LOW);
  delay(BT_PIN_CHANGE_WAIT);
  _setEnablePin(HIGH); 
  // wait for BT to boot
  delay(BT_BOOT_WAIT_TIME);
}

void BluetoothModule::powerResetFlush(){
  powerReset();
  flush();
}

void BluetoothModule::flush(){
    // read and throw away result
    Serial.println("Flushing");
    char garbage[256];
    read(garbage);
}

void BluetoothModule::_setEnablePin(bool state){
    digitalWrite(_enPin, state);
}

void BluetoothModule::_setKeyPin(bool state){
    digitalWrite(_keyPin, state);    
}

bool BluetoothModule::isValidATcommand(const char* cmd){
  int len = strlen(cmd);
  return len<2 || !(cmd[0]=='A' && cmd[1]=='T');
}

bool BluetoothModule::executeSingleATcommand(const char* cmd, char* respBuff, int setATmode){
  // Response wait timeout
  const int TIMEOUT = 2000;

  // ensure AT mode
  if (setATmode != ATmode::NONE && _currATmode != setATmode) {
    enterATmode(setATmode);
  }
  if (_currATmode == ATmode::OFF){
    Serial.println("Not in AT mode - aborting execution");
    if (respBuff!=NULL) strcpy(respBuff, "ERROR:(-0)");
    return false;
  }
  
  // check if valid AT command
  if (isValidATcommand(cmd)) {
    if (respBuff!=NULL) strcpy(respBuff, "ERROR:(-1)");
    return false;
  }

  Serial.print("Executing AT command: ");
  Serial.println(cmd);

  // ensure/add "\r\n" to command
  // send command to BT serial
  int len = strlen(cmd);
  if (len>=2 && cmd[len-2]=='\r' && cmd[len-1]=='\n') {
      _BTserial->print(cmd);
  } else{
      _BTserial->println(cmd);
  } 

  // wait for response
  unsigned long timeStart = millis();
  while(!BTserialAvailable()){
    if ((millis()-timeStart)>TIMEOUT) {
      if (respBuff!=NULL) strcpy(respBuff, "ERROR:(-2)");
      return false;
    }
  }

  // read response from BT serial and ensure ends with "OK"
  char resp[256];
  read(resp);

  // check result was OK
  bool success;
  len = strlen(resp);
  if(len>=2) success = resp[len-2]=='O' && resp[len-1]=='K';

  Serial.print("AT command was 'OK': ");
  Serial.println(success);

  // fill response buffer if applicable
  if (respBuff!=NULL){
    strcpy(respBuff, resp);
    if (success && len>=4) {
      respBuff[len-4] = 0; //-4 because '\r\nOK'
    }
  }

  if (setATmode != ATmode::NONE) // only exit if ATmode was specified (o/w it can be expected that it's handled externally)
    exitATmode();

  return success;
}

bool BluetoothModule::executeATCommands(const char* cmds[], bool resetArr[], int n, int setATmode){
  
  bool success = true;
  bool mustReset = false;

  Serial.println("Executing AT commands:");

  enterATmode(setATmode);

  char error[256];
  for(int i=0;i<n;i++){
    if(!executeSingleATcommand(cmds[i], error, ATmode::NONE)) {
      success = false;
      Serial.println(error); 
    }
    if (resetArr[i]){
      mustReset = true;
      powerResetFlush();
    }
  }

  exitATmode();

  if (mustReset) powerResetFlush();

  return success;
}

int BluetoothModule::BTserialAvailable(){
  return _BTserial->available();
}

bool BluetoothModule::waitForConnection(int timeoutSec, const char* addr){
  // address verifcation not yet implemented
  Serial.println("Waiting for Connection ... ");
  char respBuff[256];
  bool connectionVerified = false;
  unsigned long start = millis();

  executeSingleATcommand("AT+DISC");

  while ((millis()-start) < (1000*timeoutSec)){
    executeSingleATcommand("AT+STATE", respBuff, ATmode::LIMITED);
    Serial.print("Connection Response: ");
    Serial.println(respBuff);
    if (strcmp(respBuff, "+STATE:CONNECTED")==0){
      connectionVerified = true;
      break;      
    }
  }
  Serial.print("Connection Success: ");
  Serial.println(connectionVerified);

  return connectionVerified;
}

void BluetoothModule::sendData(const char* payload){
  DynamicJsonDocument json(256);
  json["type"] = packetType::DATA;
  json["data"] = payload;
  
  char buff[256];
  serializeJson(json, buff);
  
  sendJsonString(buff);
}

void BluetoothModule::sendKey(const char key){
  _BTserial->write(key);
}

bool BluetoothModule::basicConfigReset(){
  Serial.println("Resetting to basic config");
  bool success = executeATCommands(basicConfig, basicConfigResetArr, basicConfigArrSize, ATmode::LIMITED);
  return success;
}

void BluetoothModule::sendJsonString(const char *jsonString){
  // send json to specifed SoftwareSerial object
  Serial.print("Sending JSON: ");

  int i=0;
  while(jsonString[i] != '\0'){
    if (i%SERIAL_BUFFER_LENGTH/2 == 0) delay(SEND_BUFFER_PAUSE_WAIT_TIME);
    _BTserial->write(jsonString[i]);
    Serial.write(jsonString[i]);
    i++;
  }
  Serial.println();

  delay(SEND_WAIT_TIME);
}

int BluetoothModule::determinePacketType(DynamicJsonDocument &doc){
  return doc["type"];
}

void BluetoothModule::retrievePacketData(DynamicJsonDocument &doc, char* data){
  strcpy(data, doc["data"]);
}

int BluetoothModule::retrieveATcommandsLength(DynamicJsonDocument &doc){
  return doc["len"];
}

void BluetoothModule::retrieveATcommands(DynamicJsonDocument &doc, char* ATcmds[], bool resetArr[], int n){
  for (int i=0;i<n;i++){
    ATcmds[i] = doc["cmds"][i];
    resetArr[i] = doc["reset"][i];
  }
}

bool BluetoothModule::waitForACK(){
  int timeout = 2000;
  unsigned long start = millis();
  while((millis()-start)<timeout){
    char res[256];
    read(res);
    // Serial.println(res);
    StaticJsonDocument<32> doc;
    deserializeJson(doc, res);  
    // serializeJson(doc, Serial);
    if (doc.containsKey("type") && doc["type"]==(int)packetType::DATA){
      if (strcmp(doc["data"], "ACK")==0) {
        Serial.println("ACK success!");
        return true;
      }
    }
  }
  Serial.println("ACK failed - timed out!");
  return false;
}

bool BluetoothModule::sendATCommands(const char* ATcommands[], bool resetArr[], int n){
  StaticJsonDocument<256> doc;
  Serial.println("Sending AT commands to other device");
  // fill JSON with data
  doc["type"] = packetType::AT;
  doc["len"] = n;
  for (int i=0; i<n; i++){
    doc["cmds"][i] = ATcommands[i];
    doc["reset"][i] = (int)resetArr[i];
  }

  char buff[256];
  Serial.println(doc.capacity());
  serializeJson(doc, buff);

  // send JSON data through BT
  sendJsonString(buff);

  return true;
}