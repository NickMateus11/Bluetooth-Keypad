
#ifndef SETTINGS_H
#define SETTINGS_H

/* DONT FORGET TO SET THIS PROPERLY */

// If the device is to be used on battery, with power saving enabled, and using pins 0 (RX) and 1 (TX)
#define DEVICE_ATMEGA328P


#ifdef DEVICE_ATMEGA328P
#define ENABLE_POWER_SAVING
#endif

#ifdef ENABLE_POWER_SAVING
#define USE_NATIVE_SERIAL_FOR_BT
#endif

#endif