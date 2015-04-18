Linux kernel space driver for Corsair Vengeance K90 Keyboard
============================================================

A simple HID driver for the K90 keyboard. Current features are:
 - Remapping the G keys
 - Hiding other special keys
 - Change backlight brightness
 - Switch between hardware playback and software mode
 - Switch between profiles
 - Toggle the record macro LED

hid-generic takes control of the K90 keyboard, you need to unbind it (at least for the interface 0 of the keyboard).
```
sudo tee /sys/bus/hid/drivers/hid-generic/unbind <<< "0003:1B1C:1B02.XXXX"
```
Replace `XXXX` with the correct value for the first interface of the keyboard.

Parameters
----------

- **gkey_codes** An array of 18 (comma-separated) keycodes for remapping the G keys.

Sysfs
-----

- **brightness** (read/write) Change the brightness from 0 (off) to 3 (brightest).
- **macro_mode** (read/write) Switch playback mode. Values are "HW" or "SW". The value is not read from the keyboard, so initial state read are unreliable.
- **macro_record** (read/write) Toggle the red MR led. Values are "ON" or "OFF". The value is not read from the keyboard, so initial state read are unreliable.
- **current_profile** (read/write) Change the current profiles. Values are 1, 2 or 3.



