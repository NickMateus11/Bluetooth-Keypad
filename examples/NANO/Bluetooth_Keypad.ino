
#include "src/settings.h"

#include "src/Keypad.h"
#include "src/ArduinoSleep.h"
#include "src/BluetoothModule.h"

// UART setting for communicating with the bluetooth module in regular mode (AT mode is always 38400)
#define BT_BAUD 38400

// Only allow RX & TX pin setting if not using native Serial (needed if power saving enabled)
#ifndef USE_NATIVE_SERIAL_FOR_BT
#define RX_PIN 6
#define TX_PIN 7
#endif

// BT module & script params
#define EN_PIN 10
#define KEY_PIN 11
#define ROW_PIN_START 2 
#define COL_PIN_START 6 
#define SLEEP_TIMEOUT 5000

#ifndef USE_NATIVE_SERIAL_FOR_BT
BluetoothModule BTmodule = BluetoothModule(EN_PIN, KEY_PIN, BT_BAUD, HIGH, RX_PIN, TX_PIN);
#else
BluetoothModule BTmodule = BluetoothModule(EN_PIN, KEY_PIN, BT_BAUD, HIGH);
#endif

#ifdef ENABLE_POWER_SAVING
unsigned long timeSinceLastSleep;
#endif


void setup() {

  #ifndef USE_NATIVE_SERIAL_FOR_BT
  Serial.begin(9600);
  #endif

  // Begin Serial/Software Serial, set pinModes, turn on BT module
  BTmodule.startup();
  
  // Setup keypad pins to allow for key registering
  keypadInit(ROW_PIN_START, COL_PIN_START);

  // get input and test for special keys on startup
  char initial_key = get_input();

  if(initial_key == '0') BTmodule.basicConfigReset(); // reset BT config
    
  else if (initial_key == '#') BTmodule.waitForATandExecute(); // wait for and accept new config 
    
  else if (initial_key == '*'){ } // UNUSED

  #ifdef ENABLE_POWER_SAVING
  timeSinceLastSleep = millis(); // start sleep time countdown
  #endif
}

void loop() {

  #ifdef ENABLE_POWER_SAVING
  if ((millis()-timeSinceLastSleep) > SLEEP_TIMEOUT){
    enableSleep();  // sleep begins - no further code is executed until keypad press interrupt
    timeSinceLastSleep = millis(); // only after waking up - set new value
  }
  #endif

  // check for input, send key data over BT if key pressed
  char key_data = get_input();
  if (key_data) {
    BTmodule.sendKey(key_data);
    while(get_input() == key_data); // ensure button press accounted for only once
  }
}
