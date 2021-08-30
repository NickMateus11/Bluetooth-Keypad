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
    void enterATmode(int setATmode=ATmode::FULL);
    void exitATmode();
    void powerResetFlush();
    void powerReset();
    void sendData(const char* payload);
    void sendKey(const char key);
    void sendJsonString(const char *jsonString);
    void retrieveATcommands(DynamicJsonDocument &doc, char* ATcmds[], bool resetArr[], int n);
    void read(char* buff);
    void retrievePacketData(DynamicJsonDocument &doc, char* data);

    bool sendATCommands(const char* ATcommands[], bool resetArr[], int n);
    bool executeATCommands(const char* cmds[], bool resetArr[], int n, int setATmode=ATmode::LIMITED);
    bool executeSingleATcommand(const char* cmd, char* respBuff=NULL, int setATmode=ATmode::LIMITED);
    bool waitForConnection(int timeoutSec, const char* addr=NULL);
    bool isValidATcommand(const char* cmd);
    bool basicConfigReset();
    bool waitForATandExecute();
    bool waitForACK();

    int BTserialAvailable();
    int retrieveATcommandsLength(DynamicJsonDocument &doc);
    int determinePacketType(DynamicJsonDocument &doc);
    

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