
#include "src/settings.h"

#include "src/Keypad.h"
#include "src/ArduinoSleep.h"
#include "src/BluetoothModule.h"

#define BT_BAUD 38400

#ifndef USE_NATIVE_SERIAL_FOR_BT
#define RX_PIN 6
#define TX_PIN 7
#endif

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

  BTmodule.startup();
  
  keypadInit(ROW_PIN_START, COL_PIN_START);

  char initial_key = get_input();
  if(initial_key == '0'){ // reset BT config
    BTmodule.basicConfigReset();
  } 
  else if (initial_key == '#'){ // wait for and accept new config 
    BTmodule.waitForATandExecute();
  } 
  else if (initial_key == '*'){ // UNUSED

  }
  // while(get_input() == initial_key); // ensure key is released

  timeSinceLastSleep = millis(); // start sleep time countdown

}

void loop() {

  #ifdef ENABLE_POWER_SAVING
  if ((millis()-timeSinceLastSleep) > SLEEP_TIMEOUT){
    enableSleep();  // sleep begins - no further code is executed until interrupt
    timeSinceLastSleep = millis(); // only after waking up
  }
  #endif

  char key_data = get_input();
  if (key_data) {
    BTmodule.sendKey(key_data);
    while(get_input() == key_data); // ensure button press accounted for only once
  }
}
