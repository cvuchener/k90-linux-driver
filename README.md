Linux kernel space driver for Corsair Vengeance K90 Keyboard
============================================================

A simple HID driver for the K90 keyboard. Current features are:
 - Remapping the G keys
 - Hiding other special keys
 - Change backlight brightness
 - Switch between hardware playback and software mode
 - Switch between profiles
 - Toggle the record macro LED

The K90 has three USB interfaces:
 - **Interface 0** handles control messages and send HID reports for special keys and macro playback.
 - **Interface 1** HID reports for multimedia keys (Consumer Control HID usage page).
 - **Interface 2** HID reports for regular keys.

hid-generic takes control of the K90 keyboard, you need to unbind it (at least for the interface 0 of the keyboard, it works well with interface 1 and 2).
```
sudo tee /sys/bus/hid/drivers/hid-generic/unbind <<< "0003:1B1C:1B02.XXXX"
```
Replace `XXXX` with the correct value for the first interface of the keyboard.

If the modules is installed on the system, udev loads the module thanks to modules aliases but hid-generic still take over, the device can be rebound with this rule:
```
ATTRS{idVendor}=="1b1c", ATTRS{idProduct}=="1b02", GOTO="corsair_k90"
GOTO="corsair_k90_end"

LABEL="corsair_k90"
ACTION=="add", SUBSYSTEM=="hid", ATTRS{bInterfaceNumber}=="00", RUN+="/bin/sh -c 'echo $kernel > /sys/bus/hid/drivers/hid-generic/unbind; echo $kernel > /sys/bus/hid/drivers/corsair/bind'"

LABEL="corsair_k90_end"
```

Parameters
----------

- **gkey_codes** An array of 18 (comma-separated) keycodes for remapping the G keys.

Sysfs
-----

- **macro_mode** (read/write) Switch playback mode. Values are "HW" or "SW".
- **current_profile** (read/write) Change the current profiles. Values are 1, 2 or 3.

LEDs
----

The driver create two devices in the *led* class for the backlight and the macro record led, respectively named *<devicename>:blue:backlight* and *<devicename>:red:record*.

Profile
-------

This driver does not expose any ABI for writing profile. You can use the user space program at https://github.com/cvuchener/k90-send-profile.

