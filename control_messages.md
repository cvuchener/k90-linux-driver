USB Control Messages
====================

This protocol is used by:
 - Vengeance K90 (1b02)
 - Vengeance K95?
 - Gaming K40 (1b0e)

Request type use the vendor bit so it is 0x40 for out messages and 0xC0 for in messages.

General
-------

### 2 – Macro mode requests

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 2      | macro mode operations  |
| wValue       | 0x0001 | Hardware mode          |
|              | 0x0010 | Firmware update mode   |
|              | 0x0030 | Software mode          |
|              | 0x0020 | Blink MR LED           |
|              | 0x0040 | Stop MR LED            |
| wIndex       | 0      |                        |
| wLength      | 0      |                        |


### 5 – Mode query

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 5      |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 2      |                        |

Response data is:

| Byte | Value | Description |
| ---- | ----- | ----------- |
| 0    |       | Device mode |
| 1    | 0x01  | ?           |

Device mode uses the same values as the set mode request (bRequest = 2):

| Value | Mode                   |
| ----- | ---------------------- |
| 0x01  | Hardware playback mode |
| 0x10  | Firmware update mode   |
| 0x30  | Software mode          |

MR LED state does not change the mode.

Byte 1 may change after failed request (sent too quickly after another).


### 4 – Status requests

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 4      | status request         |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      |        | device dependant       |

#### K90 status format

Status is 8 byte long.

| Byte | Value | Description          |
| ---- | ----- | -------------------- |
| 0    | 0x01  | ?                    |
| 1    | 0x01  | ?                    |
| 2    | 0x00  | ?                    |
| 3    | 0x00  | ?                    |
| 4    | 0–3   | Backlight brightness |
| 5    | 0x00  | ?                    |
| 6    | 0x01  | ?                    |
| 7    | 1–3   | Current profile      |


#### K40 status format

Status is 10 byte long.

| Byte | Value | Description                       |
| ---- | ----- | --------------------------------- |
| 0    | 0x01  | ?                                 |
| 1    | 0–3   | Backlight brightness              |
| 2    |       | High byte of wValue in request 50 |
| 3    | 0x00  | ?                                 |
| 4–6  |       | Backlight color (R8G8B8)          |
| 7    | 1–3   | Current profile                   |
| 8    | 0x00  | ?                                 |
| 9    | 0–1   | Color mode (see request 56)       |


Profile
-------

### 20 – Profile switch 

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 20     | switch current profile |
| wValue       | 1–3    | New current profile    |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |

### 16 – Macro bindings 

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 16     | send macro bindings    |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      |        |                        |

Data sent is:

| Byte | Type         | Content     |
| ---- | ------------ | ----------- |
| 0    | byte         | Bind count  |
| 1–2  | int16 be     | Size of this data |
| 3–4  | int16 be     | Size of the macro data (sent with request 18) |
| 5–…  | struct array | An array of 5-byte-long bind structures |

Bind count is usually 18 for the K90 (wLength = 95), or 6 for the K40 (but, wLength = 37, the structure may be different).

Bind structure is filled with zeros when disabled. Valid bind are:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Data type: 0x10 = key usage code, 0x20 = macro |
| 1–2  | int16 be | Offset in raw data |
| 3–4  | int16 be | Length of data |

Data length must not exceed 128 (or the keyboard will block until reset).


### 18 – Send raw data 

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 18     | send raw macro data    |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      |        | variable               |

Data contains the raw macro items, key usage codes referenced in request 16.

Known macro items are:
 - Key event

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | 0x84        |
| 1    | byte     | HID usage   |
| 2    | byte     | New state: 0 = released, 1 = pressed |

 - Delay

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | 0x87        |
| 1–2  | int16 be | Delay in milliseconds |

 - End of macro

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | 0x86        |
| 1–2  | int16 be | Repeat count |


### 22 – G key roles

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 22     | send G key roles       |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      | 37     | For the Windows software, but may change |

Data sent is:

| Byte | Type         | Content     |
| ---- | ------------ | ----------- |
| 0    | byte         | Key count   |
| 1–…  | struct array | An array of 2-byte-long key role structures |

Key count is 18 for the K90 and 6 for the K40 but wLenght is 37 for both.

Key role structure is:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Key usage |
| 1    | byte     | Playback type: 1 = play once, 2 = repeat while held, 3 = repeat until pressed again |

Each element of this array is matched with the element of the same index in the bindings array. When the key whose usage is in this array is pressed the macro from the binding array is played.

The key usage seems to be read only in the sixteen first items, key usage for a non G-key may be used there. If a usage for the key G*k* is not present, it will use the *k*th item in this list anyway, so the macro will actually be bound to two keys the one whose usage is given here and the G-key of corresponding rank.

The playback is only used by macros (0x20 binding type) not by key usage bindings (0x10).

Maximum data length is 64.

Backlight
---------

### 49 – K90 Backlight brightness 

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 49     | light brightness       |
| wValue       | 0–3    | New brightness level   |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |


### 48 – K40 Backlight brightness

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 48     | light brightness       |
| wValue(high) | 0–3    | New brightness level   |
| wValue(low)  | 0      |                        |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |

### 51 – K40 Change backlight color

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 51     | light brightness       |
| wValue(high) |        | Green                  |
| wValue(low)  |        | Red                    |
| wIndex(high) |        | Target                 |
| wIndex(low)  |        | Blue                   |
| wLength      | 0      |                        |

Targets 1 to 3, set the corresponding profile color. Target 0 is also used by Corsair software.

### 56 – K40 Color mode

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 56     | light brightness       |
| wValue       | 0–1    | Color mode             |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |

Color modes are:
 - **True color** (wValue = 0)
 - **Max brightness** (wValue = 1)


### 49 – K40 Backlight animation

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 49     | light brightness       |
| wValue       | 0–2    | Animation mode         |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |

Animation modes are:
 - **Off** (wValue = 0)
 - **Pulse** (wValue = 1)
 - **Cycle** (wValue = 2). This mode requires regular sending of request 52.

### 52 – ?

Used with the “Cycle” animation.

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 52     | light brightness       |
| wValue       | 0x0000 | Animation mode         |
| wIndex       | 0x0000 |                        |
| wLength      | 9      |                        |

Data sent may be:

| Bytes | Description     |
| ----- | --------------- |
| 0–2   | Default color   |
| 3–5   | Animation color |
| 6–8   | *seems random*  |

Animation example with one request every 11 seconds:

```
00 00 ab 00 00 ab ee 6d 3a
00 00 ab 00 ab 00 7e 16 45
00 00 ab 00 ab ab 14 64 6c
00 00 ab ab 00 00 ba d5 14
00 00 ab ab 00 ab c0 23 4c
00 00 ab ab ab 00 4e cb e9
00 00 ab ab ab ab c1 a9 c8
00 00 ab 00 00 ab ec fe 8c
00 00 ab 00 ab 00 1d 30 d9
00 00 ab 00 ab ab c7 50 db
```


Firmware
--------

### 1 – ?

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, device -> host |
| bRequest     | 1      |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 0      |                        |

Sent after the firmware, maybe for resetting the device.


### 32 – ?

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 32     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 16     |                        |

Queried before switching to firmware mode.

Data for my device is: `1B 02 30 03 01 36 6C 00 80 00 00 64 00 A0 03 00`. `01 36` could be the BCD firmware version (1.36).

This data is found at offset 0x5157 in K90_1.36.bin, at 0x5167 the same data is found but with `0a 60` (K60 PID?) instead of `1b 02` (K90 PID).


### 33 – ?

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x80   | vendor, host -> device |
| bRequest     | 33     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 0      |                        |

Send before the firmware blob in firmware mode.


### 35 – Send Firmware data

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x80   | vendor, host -> device |
| bRequest     | 35     |                        |
| wValue       |        | Offset                 |
| wIndex       | 0      |                        |
| wLength      | 512    |                        |

Corsair firmware update software send the firmware in blocks of 512 bytes starting at offset 0x8000. K90 1.36 firmware size is 27 kb, so offset goes from 0x8000 to 0xEA00 by 0x200 increments.

Unknown requests
----------------

### 25 – ?

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 25     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 184    |                        |

Read data that is found at offset 0x2400 in the firmware (0xa400 in memory).

```
00002400: f1f2 f300 0000 d0d1 d2d3 d4d5 d6d7 d8d9  ................
00002410: dadb dcdd dedf e8e9 0000 cded b1b3 b0b2  ................
00002420: b4b5 b6eb ec68 696a 293a 3b3c 3d3e 3f40  .....hij):;<=>?@
00002430: 4142 4344 4546 4748 494a 4b4c 4d4e 6b00  ABCDEFGHIJKLMNk.
00002440: 00e3 e700 e400 e000 142b 0400 1d8b 351e  .........+....5.
00002450: 1a39 1664 1b8a 001f 0800 0700 0688 0020  .9.d........... 
00002460: 1517 090a 1905 2221 181c 0d0b 1011 2324  ......"!......#$
00002470: 0c30 0e00 3687 2e25 1200 0f00 3765 0026  .0..6..%....7e.&
00002480: 132f 3334 3238 2d27 0000 00e2 91e6 9000  ./3428-'........
00002490: 892a 3100 2800 0000 5f5c 592c 5351 0000  .*1.(..._\Y,SQ..
000024a0: 605d 5a62 544f 0000 615e 5b63 5556 0000  `]ZbTO..a^[cUV..
000024b0: 57e1 e552 5850 8567 0101 0000 0300 0101  W..RXP.g........
```

(At 0x24b8 that data 8-byte data corresponds to the answer to bRequest=4)


### 66 – ?

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 66     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 4      |                        |

