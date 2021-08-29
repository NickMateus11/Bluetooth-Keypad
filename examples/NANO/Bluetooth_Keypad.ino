
#include "src/Keypad.h"
#include "src/BluetoothModule.h"
#include <SoftwareSerial.h>


#define BT_BAUD 38400
#define RX_PIN 6
#define TX_PIN 7
#define EN_PIN 12
#define KEY_PIN 13
#define ROW_PIN_START 2 
#define COL_PIN_START 8 

BluetoothModule BTmodule = BluetoothModule(EN_PIN, KEY_PIN, BT_BAUD, RX_PIN, TX_PIN, HIGH);
char key_data;


void setup() {

  Serial.begin(9600);
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
  while(get_input() == initial_key); // ensure key is released
}

void loop() {
  key_data = get_input();
  if (key_data) {
    BTmodule.sendKey(key_data);
    while(get_input() == key_data); // ensure button press accounted for only once
  }
}
