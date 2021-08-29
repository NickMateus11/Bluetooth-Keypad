#ifndef BluetoothModule_h
#define BluetoothModule_h

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
        OFF,
        NONE
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
    void sendData(const String &payload);
    void sendKey(const char &key);
    void sendJsonString(const char *jsonString);
    void retrieveATcommands(DynamicJsonDocument &doc, const char* ATcmds[], bool resetArr[], int n);

    bool sendATCommands(const char* ATcommands[], bool resetArr[], int n);
    bool executeATCommands(const char* cmds[], bool resetArr[], int n, uint8_t setATmode=ATmode::LIMITED);
    bool executeSingleATcommand(const String &cmd, String *respBuff=NULL, uint8_t setATmode=ATmode::LIMITED);
    bool waitForConnection(int timeoutSec, const String &addr="");
    bool isValidATcommand(const String &cmd);
    bool basicConfigReset();
    bool waitForATandExecute();
    bool waitForACK();

    int BTserialAvailable();
    int retrieveATcommandsLength(DynamicJsonDocument &doc);
    uint8_t determinePacketType(DynamicJsonDocument &doc);
    
    String read();

    const char* retrievePacketData(DynamicJsonDocument &doc);

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