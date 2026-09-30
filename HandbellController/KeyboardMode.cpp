#include "KeyboardMode.h"
#include "DynamicHID.h"

// A plain HID keyboard as report ID 2 (the joystick is 3): a modifier byte,
// a reserved byte and six key codes. It shares the joystick's HID interface,
// so it needs no extra USB endpoint.
static const uint8_t _keyboardDescriptor[] PROGMEM = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report ID (2)
    0x05, 0x07,       //   Usage Page (Keyboard)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute): modifiers
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x03,       //   Input (Constant): reserved
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x73,       //   Logical Maximum (115)
    0x05, 0x07,       //   Usage Page (Keyboard)
    0x19, 0x00,       //   Usage Minimum (0)
    0x29, 0x73,       //   Usage Maximum (115)
    0x81, 0x00,       //   Input (Data, Array): key codes
    0xC0,             // End Collection
};

static const uint8_t KEYBOARD_REPORT_ID = 2;

// HID key codes
enum : uint8_t
{
    KEY_A = 0x04,
    KEY_F = 0x09,
    KEY_G = 0x0A,
    KEY_J = 0x0D,
    KEY_SEMICOLON = 0x33,
    KEY_F9 = 0x42,
};

// Strike detection as Handbell Manager does it with its defaults: on the Z
// axis, handstroke when it rises above 100 and backstroke when it falls
// below -600 on Handbell Manager's +/-2048 scale (Z is +/-16384 here, so
// x8), strokes alternating, and no strike within 600 ms of the last.
static const int16_t HANDSTROKE_STRIKE = 800;
static const int16_t BACKSTROKE_STRIKE = -4800;
static const uint32_t STRIKE_LOCKOUT_MS = 600;

// Keys are held briefly so every host sees a distinct press and release.
static const uint32_t KEY_HOLD_MS = 10;

// Cap buttons follow Handbell Manager's documented defaults: on the
// right-hand bell, Start/Stop (F9) and Go (G); on the left-hand bell, Bob
// (A) and Single (;).
static const uint8_t RIGHT_KEYS[3] = { KEY_J, KEY_F9, KEY_G }; // strike, button 0, button 1
static const uint8_t LEFT_KEYS[3] = { KEY_F, KEY_A, KEY_SEMICOLON };

static uint8_t _mode = KEYMODE_OFF;
static bool _expectHandstroke = true;
static uint32_t _strikeLockoutStart = 0;
static bool _strikeLocked = false;
static bool _lastButton[2];
static uint8_t _keys[6];
static uint32_t _keyPressedAt[6];
static bool _reportDirty = false;

static void PressKey(uint8_t code)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        if (_keys[i] == 0)
        {
            _keys[i] = code;
            _keyPressedAt[i] = millis();
            _reportDirty = true;
            return;
        }
    }
}

static void ReleaseHeldKeys(uint32_t now)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        if (_keys[i] != 0 && now - _keyPressedAt[i] >= KEY_HOLD_MS)
        {
            _keys[i] = 0;
            _reportDirty = true;
        }
    }
}

static void SendKeys()
{
    uint8_t report[8] = { 0, 0 };
    memcpy(&report[2], _keys, 6);
    DynamicHID().SendReport(KEYBOARD_REPORT_ID, report, sizeof(report));
    _reportDirty = false;
}

void keymode_setup(uint8_t mode)
{
    _mode = keymode_is_on(mode) ? mode : KEYMODE_OFF;
    if (_mode == KEYMODE_OFF)
        return;

    static DynamicHIDSubDescriptor node(_keyboardDescriptor, sizeof(_keyboardDescriptor));
    DynamicHID().AppendDescriptor(&node);
}

void keymode_update(int16_t zAxis, bool button0, bool button1)
{
    if (_mode == KEYMODE_OFF)
        return;

    const uint8_t* keys = _mode == KEYMODE_RIGHT ? RIGHT_KEYS : LEFT_KEYS;
    auto now = millis();

    if (_strikeLocked && now - _strikeLockoutStart >= STRIKE_LOCKOUT_MS)
        _strikeLocked = false;

    if (!_strikeLocked)
    {
        bool strike = _expectHandstroke ? zAxis > HANDSTROKE_STRIKE : zAxis < BACKSTROKE_STRIKE;
        if (strike)
        {
            PressKey(keys[0]);
            _expectHandstroke = !_expectHandstroke;
            _strikeLocked = true;
            _strikeLockoutStart = now;
        }
    }

    // Buttons type once per press
    bool buttons[2] = { button0, button1 };
    for (uint8_t b = 0; b < 2; b++)
    {
        if (buttons[b] && !_lastButton[b])
            PressKey(keys[b + 1]);
        _lastButton[b] = buttons[b];
    }

    ReleaseHeldKeys(now);
    if (_reportDirty)
        SendKeys();
}
