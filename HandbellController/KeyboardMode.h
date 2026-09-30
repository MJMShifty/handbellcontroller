// Keyboard mode: copyright (c) 2026 Mike Morton, MIT licence (see LICENSE).

#pragma once

#include <stdint.h>

// Optional keyboard output, so an eBell can ring apps that only read keys
// (a phone or tablet simulator, Ringing Room in a browser) with no Handbell
// Manager in between. The bell detects its own strikes and types a key for
// each one, alongside the usual joystick data.
//
// Off unless chosen by holding a cap button while plugging in; the choice is
// kept in EEPROM. Leave it off when using Handbell Manager, or strikes would
// be typed twice.
enum KeyMode : uint8_t
{
    KEYMODE_OFF = 0,
    KEYMODE_RIGHT = 1, // right-hand bell: rings J
    KEYMODE_LEFT = 2,  // left-hand bell: rings F
};

// Only these two values turn it on, so erased EEPROM (0xFF) reads as off.
inline bool keymode_is_on(uint8_t mode)
{
    return mode == KEYMODE_RIGHT || mode == KEYMODE_LEFT;
}

// Adds the keyboard to the USB HID descriptor when the mode is on; off, the
// bell is unchanged. Call straight after the Joystick is created, before
// any delay, so both are in place when the host reads the descriptor.
void keymode_setup(uint8_t mode);

// Call once per loop with the Z axis and the two (logical) cap buttons.
void keymode_update(int16_t zAxis, bool button0, bool button1);
