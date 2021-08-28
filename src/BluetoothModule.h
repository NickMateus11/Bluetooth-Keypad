#ifndef BluetoothModule_h
#define BluetoothModule_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

class BluetoothModule 
{

public:
    // constructor
    BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, uint8_t rxPin, uint8_t txPin, bool isOn=true);

    // public enums
    enum ATmode {
        FULL,
        LIMITED,
        OFF
    };

    enum packetType {
        AT,
        DATA
    };

    // public methods
    void startup();
    void flush();
    void enterATmode(uint8_t setATmode=ATmode::FULL);
    void exitATmode();
    void powerResetFlush();
    void powerReset();
    void send(const String &payload);
    void sendJsonDoc(DynamicJsonDocument &jsonDoc);
    void sendATCommands(String ATcommands[], bool resetArr[], int n);
    void retrieveATcommands(DynamicJsonDocument &doc, String ATcmds[], bool resetArr[], int n);

    bool executeATCommands(String cmds[], bool resetArr[], int n, uint8_t setATmode=ATmode::LIMITED);
    bool executeSingleATcommand(const String &cmd, String *respBuff=NULL, uint8_t setATmode=ATmode::LIMITED);
    bool waitForConnection(int timeoutSec, const String &addr="");
    bool isValidATcommand(const String &cmd);

    int BTserialAvailable();
    int retrieveATcommandsLength(DynamicJsonDocument &doc);
    uint8_t determinePacketType(DynamicJsonDocument &doc);

    DynamicJsonDocument convertToJsonDoc(const String &data);
    
    String read();
    String retrievePacketData(DynamicJsonDocument &doc);

private:
    // private variables
    uint8_t _enPin;
    uint8_t _keyPin;
    uint32_t _baudRate;
    SoftwareSerial *_BTserial;
    ATmode _currATmode;
    

    // private methods
    void _setEnablePin(bool state);
    void _setKeyPin(bool state);


};

#endif