#ifndef BluetoothModule_h
#define BluetoothModule_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "settings.h"

// SoftwareSerial doesn't need to be used when native Serial is being used for BT communication
#ifndef USE_NATIVE_SERIAL_FOR_BT
#include <SoftwareSerial.h>
#endif


// Defines Bluetooth methods needed for: setup, BT pairing, AT command configuration, sending data
class BluetoothModule 
{

public:
    // constructor
    BluetoothModule(uint8_t enPin, uint8_t keyPin, uint32_t baudRate, bool defaultState=HIGH, uint8_t rxPin=2, uint8_t txPin=3);

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

    // Set pinModes, begin Serial/SoftwareSerial w/ specified baudrate, turns BT module ON if necessary
    void startup();

    // reads BT serial and discrads the result
    void flush();

    // Enter AT mode (command mode) either with full (disconnects and power cycles) or limited (maintains connection) access
    void enterATmode(uint8_t setATmode=ATmode::FULL);

    // Exits any AT mode type the device is in
    void exitATmode();

    // Cycle power and flush serial
    void powerResetFlush();

    // Cycle power to BT module through EN pin
    void powerReset();

    // Send data in JSON format {"type":packetType::DATA, "data": <payload str>}
    void sendData(const String &payload);

    // Send single char through BT
    void sendKey(const char &key);
    
    // Sends JSON string through BT
    void sendJsonString(const char *jsonString);

    // Retreive AT commands from JSON - stores result in ATcmds & resetArr
    void retrieveATcommands(DynamicJsonDocument &doc, const char* ATcmds[], bool resetArr[], int n);

    // Package AT commands into JSON - send over BT
    bool sendATCommands(const char* ATcommands[], bool resetArr[], int n);
    
    // execute multiple AT commands and return if all were successful
    bool executeATCommands(const char* cmds[], bool resetArr[], int n, uint8_t setATmode=ATmode::LIMITED);
    
    // enter AT mode specified - execute single AT command - return if successful
    bool executeSingleATcommand(const String &cmd, String *respBuff=NULL, uint8_t setATmode=ATmode::LIMITED);
    
    // wait for BT to be in connected state
    bool waitForConnection(int timeoutSec, const String &addr="");
    
    // validate that AT command is legit (starts with 'AT')
    bool isValidATcommand(const String &cmd);

    // Reset BT module to basic config settings
    bool basicConfigReset();

    // Wait for incoming AT command JSON - parse and execute the commands
    bool waitForATandExecute();

    // Wait for 'ACK' message received - timeout is 2000ms
    bool waitForACK();

    // check if BT serial has any bytes available
    int BTserialAvailable();

    // parse JSON and return the specified number of AT commands
    int retrieveATcommandsLength(DynamicJsonDocument &doc);

    // return packet type - found in the JSON 'type' field
    uint8_t determinePacketType(DynamicJsonDocument &doc);
    
    // read from the Serial/SoftwareSerial buffer - allow small delay to allow incoming messages to arrive in full
    String read(bool *isOK=nullptr);

    // parse JSON and retrieve data from the 'data' field
    const char* retrievePacketData(DynamicJsonDocument &doc);

private:
    // private variables
    uint8_t _enPin;
    uint8_t _keyPin;
    uint32_t _baudRate;

    uint8_t _rxPin;
    uint8_t _txPin;
    bool _defaultState;
    #ifndef USE_NATIVE_SERIAL_FOR_BT
    SoftwareSerial *_BTserial;
    #else
    HardwareSerial *_BTserial;
    #endif
    ATmode _currATmode;
    

    // private methods
    void _setEnablePin(bool state);
    void _setKeyPin(bool state);


};

#endif