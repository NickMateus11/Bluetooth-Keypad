#ifndef ARDUINO_SLEEP_H
#define ARDUINO_SLEEP_H

// enable slow power mode on MCU - enables interrupts to wake up when keypad is pressed
void enableSleep();

// wakes up the MCU and disables keypad interrupts
void disableSleep();

#endif