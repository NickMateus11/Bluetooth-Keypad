
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

String BluetoothModule::read(){
  char buff[256];
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
  buff[idx] = '\0';

  Serial.println("Read Results: " + (String)buff);
  
  return String(buff);
}

void BluetoothModule::enterATmode(uint8_t setATmode){
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
    read();
}

void BluetoothModule::_setEnablePin(bool state){
    digitalWrite(_enPin, state);
}

void BluetoothModule::_setKeyPin(bool state){
    digitalWrite(_keyPin, state);    
}

bool BluetoothModule::isValidATcommand(const String &cmd){
  return (cmd.length()<2 || strcmp(cmd.substring(0,2).c_str(), "AT")) != 0;
}

bool BluetoothModule::executeSingleATcommand(const String &cmd, String *respBuff, uint8_t setATmode){
  // Response wait timeout
  const int TIMEOUT = 3000;

  // ensure AT mode
  if (_currATmode != setATmode) {
    enterATmode(setATmode);
  }
  if (_currATmode == ATmode::OFF){
    Serial.println("Not in AT mode - aborting execution");
    if (respBuff!=NULL) 
      *respBuff = "ERROR:(-0)";
    return false;
  }
  
  // check if valid AT command
  if (isValidATcommand(cmd)) {
    if (respBuff!=NULL) 
      *respBuff = "ERROR:(-1)";
    return false;
  }

  Serial.println("Executing AT command: " + cmd);
  // ensure/add "\r\n" to command
  // send command to BT serial
  if (strcmp(cmd.substring(cmd.length()-2).c_str(), "\r\n")==0) {
      _BTserial->print(cmd);
  } else{
      _BTserial->print(cmd+"\r\n");
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
  Serial.println("AT command was 'OK': " + (String)success);

  // fill response buffer if applicable
  if (respBuff!=NULL){
    if (success && resp.length()>2) {
      *respBuff = resp.substring(0,resp.length()-4); //-4 because '\r\nOK'
    } else {
      *respBuff = resp;
    }
  }

  exitATmode();

  return success;
}

bool BluetoothModule::executeATCommands(String cmds[], bool resetArr[], int n, uint8_t setATmode){
  Serial.println("Executing Many AT commands");
  String error;
  for(int i=0;i<n;i++){
    if(!executeSingleATcommand(cmds[i], &error, setATmode)) {
      Serial.println(error); 
    }
    if (resetArr[i]){
      powerResetFlush();
    }
  }
}

int BluetoothModule::BTserialAvailable(){
  return _BTserial->available();
}

bool BluetoothModule::waitForConnection(int timeoutSec, const String &addr){
  // address verifcation not yet implemented
  Serial.println("Waiting for Connection ... ");
  String respBuff;
  bool connectionVerified = false;
  unsigned long start = millis();
  while ((millis()-start) < (1000*timeoutSec)){
    executeSingleATcommand("AT+STATE", &respBuff, ATmode::LIMITED);
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
  Serial.println("Connection Success: " + (String)connectionVerified);
  return connectionVerified;
}

void BluetoothModule::send(const String &payload){
  DynamicJsonDocument json(256);
  json["type"] = packetType::DATA;
  json["data"] = payload;

  sendJsonDoc(json);
}

void BluetoothModule::sendJsonDoc(DynamicJsonDocument &jsonDoc){
  // send json to specifed SoftwareSerial object
  Serial.print("Sending JSON: ");
  // serializeJson(jsonDoc, Serial);
  // Serial.println();

  String BTserialBuff;
  serializeJson(jsonDoc, BTserialBuff);

  for (int i=0; i<BTserialBuff.length(); i++){
    if (i%SERIAL_BUFFER_LENGTH/2 == 0) 
      delay(SEND_BUFFER_PAUSE_WAIT_TIME);
    _BTserial->write(BTserialBuff[i]);
    Serial.write(BTserialBuff[i]);
  }
  Serial.println();

  delay(SEND_WAIT_TIME);
}

DynamicJsonDocument BluetoothModule::convertToJsonDoc(const String &data){
  Serial.println("Parsing JSON string: " + data);
  DynamicJsonDocument doc(256);
  deserializeJson(doc, data);
  return doc;  
}

uint8_t BluetoothModule::determinePacketType(DynamicJsonDocument &doc){
  return doc["type"];
}

String BluetoothModule::retrievePacketData(DynamicJsonDocument &doc){
  return doc["data"];
}

int BluetoothModule::retrieveATcommandsLength(DynamicJsonDocument &doc){
  return doc["length"];
}

void BluetoothModule::retrieveATcommands(DynamicJsonDocument &doc, String ATcmds[], bool resetArr[], int n){
  const char *tmpStr;

  for (int i=0;i<n;i++){
    tmpStr = doc["cmds"][i];
    ATcmds[i] = (String)tmpStr;

    resetArr[i] = doc["resetArr"][i];
  }
}

void BluetoothModule::sendATCommands(String ATcommands[], bool resetArr[], int n){
  DynamicJsonDocument json(256);
  Serial.println("Sending AT commands to other device");
  // fill JSON with data
  json["type"] = packetType::AT;
  json["length"] = n;
  for (int i=0; i<n; i++){
    json["cmds"][i] = ATcommands[i];
    json["resetArr"][i] = resetArr[i];
  }

  // send JSON data through BT
  sendJsonDoc(json);
}