
#include <Arduino.h>
#include "Keypad.h"
#include "settings.h"

#ifdef ENABLE_POWER_SAVING
#include "ArduinoSleep.h"
#include <EnableInterrupt.h>
#endif


char key_map[4][4] = { 
  {'1','2','3','A'}, 
  {'4','5','6','B'}, 
  {'7','8','9','C'}, 
  {'*','0','#','D'} 
};

int colStart, rowStart;

void keypadInit(int r, int c){
  colStart = c;
  rowStart = r;
  for (int i = 0; i < 4; i++) {
    pinMode(rowStart + i, OUTPUT);   // configure rows
    digitalWrite(rowStart + i, LOW);

    pinMode(colStart + i, INPUT_PULLUP); // configure cols
  }
}

/*
   get_input() - checks if keypad button is pressed
   returns char of pressed button or null character
   if nothing is pressed
*/
char get_input() {
  char button = '\0';
  bool intial_state[4];

  delay(50); // debounce

  for (int i = 0; i < 4; i++) {
    intial_state[i] = (digitalRead(rowStart + i) == HIGH);
    digitalWrite(rowStart + i, HIGH);
  }

  for (int i = 0; i < 4; i++) {
    digitalWrite(rowStart + i, LOW);
    for (int j = 0; j < 4; j++) {
      if ( !digitalRead(colStart + j) ) {
        button = key_map[i][j];
      }
    }
    digitalWrite(rowStart + i, HIGH);
  }

  for (int i = 0; i < 4; i++) {
    digitalWrite(rowStart + i, intial_state[i] ? HIGH : LOW);
  }
  return button;
}

#ifdef ENABLE_POWER_SAVING
void enable_keypad_int() {
  for (int i = 0; i < 4; i++) {
    enableInterrupt(colStart + i, keypad_ISR, FALLING);
  }
}

void disable_keypad_int() {
  for (int i = 0; i < 4; i++) {
    disableInterrupt(colStart + i);
  }
}

void keypad_ISR() {
  disableSleep();
  disable_keypad_int();
}
#endif