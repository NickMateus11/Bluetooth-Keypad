

/*
 * ENABLE INTERRUPTS LIBRARY AND 
 * SOFTWARE SERIAL LIBRARY DON'T 
 * WORK TOGETHER 
 */

//#include <EnableInterrupt.h>  // library to handle 'Pin Change Interrupts'
#include <avr/sleep.h>
#include <SoftwareSerial.h>

#include "src/test.h"

 
#define BT_BAUD 38400
#define ARDUINO_RX 6
#define ARDUINO_TX 7

#define ROW_PIN_START 2 //pins 2-5 are keypad 1-4 (right-to-left)
#define COL_PIN_START 8 //pins 8-11 are keypad 5-8 (right-to-left)


/*  
 *  Arduino "RX" -> BT "TX" 
 *  Arduino "TX" -> BT "RX" (needs voltage divider)
*/ 
SoftwareSerial BTserial(ARDUINO_RX, ARDUINO_TX); // Arduino "RX" | Arduino "TX"
char key_map[4][4] = { 
  {'1','2','3','A'}, 
  {'4','5','6','B'}, 
  {'7','8','9','C'}, 
  {'*','0','#','D'} 
};
volatile bool button_flag = false;
char key_data;


void setup() {

  for (int i=0; i<4; i++) {
    pinMode(ROW_PIN_START + i, OUTPUT);   // configure rows
    digitalWrite(ROW_PIN_START + i, LOW);
    
    pinMode(COL_PIN_START + i, INPUT_PULLUP); // configure cols
  }
//  enable_keypad_int();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(9600);
  BTserial.begin(BT_BAUD);
}

void loop() {
//  noInterrupts();
//  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//  sleep_enable();
//  
//  enable_keypad_int();
//
//  interrupts();
//  sleep_mode();
  
//  if(button_flag){
//    button_flag = false;
//    char button = get_input();
//    if (button) {
//      Serial.println(button);
//      BTserial.write(key_data); 
//    }
//    delay(50);
//  }

  key_data = get_input();
  if(key_data){
    Serial.write(key_data);
    BTserial.write(key_data); 
    delay(50);
    while(get_input());
  }
}


//void enable_keypad_int() {
//  for (int i=0; i<4; i++) {
//    enableInterrupt(COL_PIN_START + i, keypad_ISR, FALLING);
//  }
//}
//
//
//void disable_keypad_int() {
//  for (int i=0; i<4; i++) {
//    disableInterrupt(COL_PIN_START + i);
//  }
//}


//void keypad_ISR () {
//  sleep_disable();
//  disable_keypad_int();
//  button_flag = true;
//}


/*
 * get_input() - checks if keypad button is pressed
 * returns char of pressed button or null character 
 * if nothing is pressed
 */
char get_input() {
  char button = '\0';
  bool intial_state[4];
  for (int i=0; i<4; i++) {
    intial_state[i] = (digitalRead(ROW_PIN_START + i) == HIGH);
    digitalWrite(ROW_PIN_START + i, HIGH);
  }
  for (int i=0; i<4; i++) {
    digitalWrite(ROW_PIN_START + i, LOW);
    for (int j=0; j<4; j++){
      if ( !digitalRead(COL_PIN_START + j) ) {
        button = key_map[i][j];   
      }
    }
    digitalWrite(ROW_PIN_START + i, HIGH);
  }  
  for (int i=0; i<4; i++) {
    digitalWrite(ROW_PIN_START + i, intial_state[i]? HIGH:LOW);
  }  
  return button;
}
