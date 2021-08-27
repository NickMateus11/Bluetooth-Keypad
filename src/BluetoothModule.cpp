
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "BluetoothModule.h"


#define BT_BOOT_WAIT_TIME 1000
#define READ_EXTRA_WAIT_TIME 100
#define SEND_WAIT_TIME (READ_WAIT_TIME/2)
#define BT_PIN_CHANGE_WAIT 10


BluetoothModule::BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, uint8_t rxPin, uint8_t txPin){
    _enPin = enPin;
    _keyPin = keyPin;
    _baudRate = baudRate;
    *_BTserial = SoftwareSerial(rxPin, txPin);

    pinMode(_enPin, OUTPUT);
    _setEnablePin(LOW);
    pinMode(_keyPin, OUTPUT);
    _setKeyPin(LOW);
}

void BluetoothModule::startup(){
  _setEnablePin(HIGH);
  _setKeyPin(LOW);
}

String BluetoothModule::read(){
  char buff[256];
  int idx=0;
  bool courtesyWait=true;
  while (true){
    // read BT buffer and store char
    if (BTserialAvailable()){
      buff[idx++] = (*_BTserial).read();
      delay(2);
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
  buff[idx] = '\0';
  
  return String(buff);
}

void BluetoothModule::enterATmode(uint8_t setATmode){
  // power cycle and enable AT mode
  if (setATmode==ATmode::FULL) {
    _setEnablePin(LOW);
    _setKeyPin(HIGH);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);
  // limited AT mode - maintain connection
  } else if (setATmode==ATmode::LIMITED) {
    _setKeyPin(HIGH);
    delay(100);
  }

  _currATmode = static_cast<ATmode>(setATmode);

  flush();
}

void BluetoothModule::exitATmode(){
  // disable current AT mode - either full or limited
  if (_currATmode == ATmode::FULL) {
    _setEnablePin(LOW);
    _setKeyPin(LOW);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);
  } else {
    _setKeyPin(LOW);
    delay(100);
  }

  _currATmode = ATmode::OFF;

  flush();
}

void BluetoothModule::powerReset(){
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
    read();
}

void BluetoothModule::_setEnablePin(bool state){
    digitalWrite(_enPin, state);
}

void BluetoothModule::_setKeyPin(bool state){
    digitalWrite(_keyPin, state);    
}

bool BluetoothModule::executeSingleATcommand(const String &cmd, String *respBuff, uint8_t setATmode){
  // Response wait timeout
  const int TIMEOUT = 3000;

  enterATmode(setATmode);
  
  // check if valid AT command
  if (cmd.length()<2 || strcmp(cmd.substring(0,2).c_str(), "AT") != 0) {
    if (respBuff!=NULL) 
      *respBuff = "ERROR:(-1)";
    return false;
  }

  // ensure/add "\r\n" to command
  // send command to BT serial
  if (strcmp(cmd.substring(cmd.length()-2).c_str(), "\r\n")==0) {
      send(cmd);
  } else{
      send(cmd+"\r\n");
  } 

  // wait for response
  unsigned long timeStart = millis();
  while(!BTserialAvailable()){
    if ((millis()-timeStart)>TIMEOUT) {
      if (respBuff!=NULL) *respBuff = "ERROR:(-2)";
      return false;
    }
  }

  // read response from BT serial and ensure ends with "OK"
  String resp = read();
  bool success = strcmp(resp.substring(resp.length()-2).c_str(), "OK")==0;

  // fill response buffer if applicable
  if (respBuff!=NULL){
    if (success && resp.length()>2) {
      *respBuff = resp.substring(0,resp.length()-2);
    } else {
      *respBuff = resp;
    }
  }

  exitATmode();

  return success;
}

bool BluetoothModule::executeATCommands(String cmds[], bool resetArr[], int n, uint8_t setATmode){
  String error;
  for(int i=0;i<n;i++){
    if(!executeSingleATcommand(cmds[i], &error)) {
      Serial.println(error); 
    }
    if (resetArr[i]){
      powerResetFlush();
    }
  }
}

bool BluetoothModule::BTserialAvailable(){
  return (*_BTserial).available();
}

bool BluetoothModule::waitForConnection(int timeoutSec, const String &addr){
  // address verifcation not yet implemented
  String respBuff;
  bool connectionVerified = false;
  unsigned long start = millis();
  enterATmode(ATmode::LIMITED);
  while ((millis()-start) < (1000*timeoutSec)){
    executeSingleATcommand("AT+STATE", &respBuff);
    Serial.println(respBuff);
    if (strcmp(respBuff.c_str(), "+STATE:CONNECTED")==0){
      if (addr.length()>0){
        // verify connection address
      } else {
        connectionVerified = true;
      }
      break;      
    }
  }
  exitATmode();
  return connectionVerified;
}

void BluetoothModule::send(const String &msg){
  (*_BTserial).print(msg);
  // delay(SEND_WAIT_TIME);
}

void BluetoothModule::sendJsonData(DynamicJsonDocument &jsonDoc){
  // send json to specifed SoftwareSerial object
  serializeJson(jsonDoc, *_BTserial);
}

DynamicJsonDocument BluetoothModule::parseJsonData(const String &data){
  DynamicJsonDocument doc(128);
  deserializeJson(doc, data);
  return doc;  
}

void BluetoothModule::sendATCommands(String ATcommands[], bool resetArr[], int n){
  DynamicJsonDocument json(64); // BTserial buffer is only 64 bytes

  // fill JSON with data
  json["type"] = "AT";
  json["length"] = n;
  for (int i=0; i<n; i++){
    json["ATcmds"][i] = ATcommands[i];
    json["resetArr"][i] = resetArr[i];
  }

  // send JSON data through BT
  sendJsonData(json);
}