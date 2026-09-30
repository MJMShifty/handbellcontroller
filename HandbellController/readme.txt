Handbell Controller

This Windows utility flashes an Arduino-based dummy handbell with code required to support an Arduino/Accelerometer combination as a handbell controller that
can be used with Handbell Manager (https://handbellmanager.changeringing.co.uk) or Handbell Stadium (https://handbellstadium.org).

To run, unzip the contents into a folder and run upload.bat at the command prompt (or upload.ps1 in powershell). The utility will not run if more than one
dummy handbell (Arduino) is connected.

The following are supported:

Accelerometer boards:
- LIS3DH
- MPU6050
- ADXL345

Joystick axis 
- X
- Y
- Z

for the MPU6050 calculated Yaw, Pitch and Roll (in place the Gyro values) are provided on the following axis
- Rx
- Ry
- Rz

two joystick buttons
- B0
- B1

Correct configuration of the Arduino can be confirmed by observing the Arduino pin17 Red LED that shows up to 5 flashes followed by a pause. 
These correspond to:
1 - B0 pressed
2 - B1 pressed
3 - X value difference
4 - Y value difference
5 - Z value difference
6 - No flash

All five flashes should occur repeatedly when the two buttons are held down.

Keyboard mode (optional)

A bell can also detect its own strikes and type a key for each one, so it can ring apps that read keys, such as a simulator on
an iPad or iPhone or Ringing Room in a browser, without Handbell Manager. Keyboard mode is off unless chosen, so existing setups
are unaffected.

In keyboard mode the bell shows the computer a keyboard only, with no joystick: iPadOS ignores the keys of a device that also
declares a joystick. So a bell is either a keyboard or a joystick, chosen when it is plugged in, as below. Use joystick mode
(keyboard mode off) for Handbell Manager, Handbell Stadium and anything else that reads the joystick.

To choose, hold a cap button while plugging the bell in, until the LED flashes:
- B0 alone - right-hand bell: strikes type J, B0 types F9 (Start/Stop), B1 types G (Go) (1 flash)
- B1 alone - left-hand bell: strikes type F, B0 types A (Bob), B1 types ; (Single) (2 flashes)
- both - keyboard mode off: joystick only, as without keyboard mode (3 flashes)

The choice is saved, so it only needs doing once, and a bell plugged in without a button held keeps its last mode.

Strikes are detected on the Z axis with Handbell Manager's default settings: handstroke when Z rises above 800, backstroke when
it falls below -4800 (100 and -600 on Handbell Manager's scale), strokes alternating, at most one strike per 600 ms.

The Arduino core asks the host for 500 mA, far more than a bell draws. For tablets, which are strict about bus power, it can be
built asking for 100 mA by adding -DUSB_CONFIG_POWER=100 to the compiler flags, e.g. with arduino-cli:
  --build-property compiler.cpp.extra_flags=-DUSB_CONFIG_POWER=100 --build-property compiler.c.extra_flags=-DUSB_CONFIG_POWER=100
(The iPad accepted a bell once its joystick was hidden in keyboard mode; whether 100 mA is also needed wasn't isolated.)
