
#include "BluetoothModule.h"
#include <Arduino.h>
#include "settings.h"

#ifndef USE_NATIVE_SERIAL_FOR_BT
#define DEBUG_MODE
#endif

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


BluetoothModule::BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, bool defaultState, uint8_t rxPin, uint8_t txPin){
    _enPin        = enPin;
    _keyPin       = keyPin;
    _baudRate     = baudRate;
    _rxPin        = rxPin;
    _txPin        = txPin;
    _defaultState = defaultState;
}

void BluetoothModule::startup(){
    
  #ifndef USE_NATIVE_SERIAL_FOR_BT
  _BTserial = new SoftwareSerial(_rxPin, _txPin);
  #else
  _BTserial = (HardwareSerial*)&Serial;
  #endif
  _BTserial->begin(_baudRate);

  pinMode(_enPin, OUTPUT);
  _setEnablePin(_defaultState);
  pinMode(_keyPin, OUTPUT);
  _setKeyPin(LOW);

  #ifdef DEBUG_MODE
  Serial.println("-- Startup Complete --");
  #endif
}

String BluetoothModule::read(bool *isOK){
  char buff[256];
  int idx=0;
  bool courtesyWait=true;
  
  #ifdef DEBUG_MODE
  Serial.println("Reading");
  #endif

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

  #ifdef DEBUG_MODE
  Serial.print("Read Results: ");
  Serial.println(buff);
  #endif

  if(isOK != nullptr && idx>=2){
    *isOK = buff[idx-2]=='O' && buff[idx-1]=='K';
  }
  
  return buff;
}

bool BluetoothModule::waitForATandExecute(){
  flush();

  #ifdef DEBUG_MODE
  Serial.println("Waiting for AT command JSON data");  
  #endif
  
  String incomingStr = "";
  while(!incomingStr.length() || (incomingStr[0]!='{' && incomingStr[incomingStr.length()-1]!='}')){
    incomingStr = read();
  }

  DynamicJsonDocument rawJson(256);
  deserializeJson(rawJson, incomingStr);

  if (determinePacketType(rawJson) != BluetoothModule::packetType::AT) return false;

  // Return acknowledgment of AT commands receieved
  sendData("ACK");

  #ifdef DEBUG_MODE
  Serial.println("Decoding AT commands from JSON");
  #endif
  
  int n = retrieveATcommandsLength(rawJson);
  const char* ATcmds[n];
  bool resetArr[n];
  retrieveATcommands(rawJson, ATcmds, resetArr, n);
  return executeATCommands(ATcmds, resetArr, n, ATmode::FULL);
}

void BluetoothModule::enterATmode(uint8_t setATmode){
  // power cycle and enable AT mode
  if (setATmode==ATmode::FULL) {
    
    #ifdef DEBUG_MODE
    Serial.println("Entering Full AT mode");
    #endif

    _setEnablePin(LOW);
    _setKeyPin(HIGH);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);

  // limited AT mode - maintain connection
  } else if (setATmode==ATmode::LIMITED) {

    #ifdef DEBUG_MODE
    Serial.println("Entering LIMITED AT mode");
    #endif

    _setKeyPin(HIGH);
    delay(100);

  } else return;

  _currATmode = static_cast<ATmode>(setATmode);

  flush();
}

void BluetoothModule::exitATmode(){
  // disable current AT mode - either full or limited
  if (_currATmode == ATmode::FULL) {
    
    #ifdef DEBUG_MODE
    Serial.println("Exiting Full AT mode");
    #endif
    
    _setEnablePin(LOW);
    _setKeyPin(LOW);
    delay(BT_PIN_CHANGE_WAIT);
    _setEnablePin(HIGH);
    delay(BT_BOOT_WAIT_TIME);

  } else if (_currATmode == ATmode::LIMITED){

    #ifdef DEBUG_MODE
    Serial.println("Exiting LIMITED AT mode");
    #endif

    _setKeyPin(LOW);
    delay(100);

  }

  _currATmode = ATmode::OFF;

  // flush();
}

void BluetoothModule::powerReset(){

  #ifdef DEBUG_MODE
  Serial.println("Power Resetting");
  #endif

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
    #ifdef DEBUG_MODE
    Serial.println("Flushing");
    #endif

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
  const int TIMEOUT = 2000;

  // ensure AT mode
  if (setATmode != ATmode::NONE && _currATmode != setATmode) {
    enterATmode(setATmode);
  }
  if (_currATmode == ATmode::OFF){
    #ifdef DEBUG_MODE
    Serial.println("Not in AT mode - aborting execution");
    #endif
    
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

  #ifdef DEBUG_MODE
  Serial.println("Executing AT command: " + cmd);
  #endif

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
  bool success;
  String resp = read(&success);
  // sometimes read() returns with destroyed String value???
  // Serial.println(resp);
  // Serial.println(resp.length());

  #ifdef DEBUG_MODE
  Serial.println("AT command was 'OK': " + (String)success);
  #endif

  // fill response buffer if applicable
  if (respBuff!=NULL){
    if (success && resp.length()>2) {
      *respBuff = resp.substring(0,resp.length()-4); //-4 because '\r\nOK'
    } else {
      *respBuff = resp;
    }
  }

  if (setATmode != ATmode::NONE) // only exit if ATmode was specified (o/w it can be expected that it's handled externally)
    exitATmode();

  return success;
}

bool BluetoothModule::executeATCommands(const char* cmds[], bool resetArr[], int n, uint8_t setATmode){
  
  bool success = true;
  bool mustReset = false;

  #ifdef DEBUG_MODE
  Serial.println("Executing AT commands:");
  #endif

  enterATmode(setATmode);

  String error;
  for(int i=0;i<n;i++){
    if(!executeSingleATcommand(cmds[i], &error, ATmode::NONE)) {
      success = false;

      #ifdef DEBUG_MODE
      Serial.println(error); 
      #endif

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

bool BluetoothModule::waitForConnection(int timeoutSec, const String &addr){
  // address verifcation not yet implemented
  #ifdef DEBUG_MODE
  Serial.println("Waiting for Connection ... ");
  #endif

  String respBuff;
  bool connectionVerified = false;
  unsigned long start = millis();

  executeSingleATcommand("AT+DISC");

  while ((millis()-start) < (1000*timeoutSec)){
    executeSingleATcommand("AT+STATE", &respBuff, ATmode::LIMITED);
    
    #ifdef DEBUG_MODE
    Serial.println(respBuff);
    #endif
    
    if (strcmp(respBuff.c_str(), "+STATE:CONNECTED")==0){
      connectionVerified = true;
      break;      
    }
  }

  #ifdef DEBUG_MODE
  Serial.println("Connection Success: " + (String)connectionVerified);
  #endif

  return connectionVerified;
}

void BluetoothModule::sendData(const String &payload){
  DynamicJsonDocument json(256);
  json["type"] = packetType::DATA;
  json["data"] = payload;
  
  char buff[256];
  serializeJson(json, buff);
  
  sendJsonString(buff);
}

void BluetoothModule::sendKey(const char &key){
  _BTserial->write(key);
}

bool BluetoothModule::basicConfigReset(){
  #ifdef DEBUG_MODE
  Serial.println("Resetting to basic config");
  #endif
  
  bool success = executeATCommands(basicConfig, basicConfigResetArr, basicConfigArrSize, ATmode::LIMITED);
  return success;
}

void BluetoothModule::sendJsonString(const char *jsonString){
  // send json to specifed SoftwareSerial object
  #ifdef DEBUG_MODE
  Serial.print("Sending JSON: ");
  #endif

  int i=0;
  while(jsonString[i] != '\0'){
    if (i%SERIAL_BUFFER_LENGTH/2 == 0) delay(SEND_BUFFER_PAUSE_WAIT_TIME);
    _BTserial->write(jsonString[i]);
    
    #ifdef DEBUG_MODE
    Serial.write(jsonString[i]);
    #endif
    
    i++;
  }

  #ifdef DEBUG_MODE
  Serial.println();
  #endif

  delay(SEND_WAIT_TIME);
}

uint8_t BluetoothModule::determinePacketType(DynamicJsonDocument &doc){
  return doc["type"];
}

const char* BluetoothModule::retrievePacketData(DynamicJsonDocument &doc){
  return doc["data"];
}

int BluetoothModule::retrieveATcommandsLength(DynamicJsonDocument &doc){
  return doc["len"];
}

void BluetoothModule::retrieveATcommands(DynamicJsonDocument &doc, const char* ATcmds[], bool resetArr[], int n){
  const char *tmpStr;

  for (int i=0;i<n;i++){
    tmpStr = doc["cmds"][i];
    ATcmds[i] = tmpStr;

    resetArr[i] = doc["reset"][i];
  }
}

bool BluetoothModule::waitForACK(){
  int timeout = 2000;
  unsigned long start = millis();
  while((millis()-start)<timeout){
    String res = read();
    StaticJsonDocument<32> doc;
    deserializeJson(doc, res);  
    if (doc.containsKey("type") && doc["type"]==(int)packetType::DATA){
      if (strcmp(doc["data"], "ACK")==0) {

        #ifdef DEBUG_MODE
        Serial.println("ACK success!");
        #endif

        return true;
      }
    }
  }

  #ifdef DEBUG_MODE
  Serial.println("ACK failed - timed out!");
  #endif

  return false;
}

bool BluetoothModule::sendATCommands(const char* ATcommands[], bool resetArr[], int n){
  StaticJsonDocument<256> doc;

  #ifdef DEBUG_MODE
  Serial.println("Sending AT commands to other device");
  #endif

  // fill JSON with data
  doc["type"] = packetType::AT;
  doc["len"] = n;
  for (int i=0; i<n; i++){
    doc["cmds"][i] = ATcommands[i];
    doc["reset"][i] = (int)resetArr[i];
  }

  char buff[256];
  
  #ifdef DEBUG_MODE
  Serial.println(doc.capacity());
  #endif
  
  serializeJson(doc, buff);

  // send JSON data through BT
  sendJsonString(buff);

  return true;
}