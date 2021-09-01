#ifndef KEYPAD_H
#define KEYPAD_H


// initialize keypad with row and column start pins, set pinModes - rows set LOW, cols are active LOW and use internal PULL-UP 
void keypadInit(int r, int c);

// get the key that is pressed (debounce as well) - return NULL character if nothing pressed
char get_input();

// enables interrupts for when the keypad is FALLING edge
void enable_keypad_int();

// disables all keypad interrupts
void disable_keypad_int();

// Interrupt Service Routine for when keypad button is pressed while MCU is asleep - wakes up MCU and disables further keypad ints
void keypad_ISR();

#endif