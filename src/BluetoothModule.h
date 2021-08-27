#ifndef BluetoothModule_h
#define BluetoothModule_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

class BluetoothModule 
{

public:
    // constructor
    BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, uint8_t rxPin, uint8_t txPin);

    // public enums
    enum ATmode {
        FULL,
        LIMITED,
        OFF,
        NONE
    };

    // public methods
    void startup();
    void flush();
    void enterATmode(uint8_t setATmode=ATmode::FULL);
    void exitATmode();
    void powerResetFlush();
    void powerReset();
    void send(const String &msg);
    void sendJsonData(DynamicJsonDocument &jsonDoc);
    void sendATCommands(String ATcommands[], bool resetArr[], int n);

    bool executeATCommands(String cmds[], bool resetArr[], int n, uint8_t setATmode=ATmode::NONE);
    bool executeSingleATcommand(const String &cmd, String *respBuff=NULL, uint8_t setATmode=ATmode::NONE);
    bool BTserialAvailable();
    bool waitForConnection(int timeoutSec, const String &addr="");

    DynamicJsonDocument parseJsonData(const String &data);
    
    String read();

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