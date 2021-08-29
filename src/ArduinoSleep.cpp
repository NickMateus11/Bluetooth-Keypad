
#include "ArduinoSleep.h"
#include <avr/sleep.h>
#include "Keypad.h"


void enableSleep(){
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  enable_keypad_int();

  sleep_mode();
}

void disableSleep(){
  sleep_disable();
}