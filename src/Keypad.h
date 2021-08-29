#ifndef KEYPAD_H
#define KEYPAD_H

void keypadInit(int r, int c);
char get_input();
void enable_keypad_int();
void disable_keypad_int();
void keypad_ISR();

#endif