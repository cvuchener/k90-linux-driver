USB Control Messages for the K90 keyboard
=========================================

Request type use the vendor bit so it is 0x40 for out messages and 0xC0 for in messages.

? (bRequest = 1)
----------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, device -> host |
| bRequest     | 1      |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 0      |                        |

Sent after the firmware, maybe for resetting the device.


Macro mode requests (bRequest = 2)
----------------------------------

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


Status requests (bRequest = 4)
----------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 4      | status request         |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 8      |                        |

Response data is:

| Byte | Value | Description |
| ---- | ----- | ----------- |
| 0    | 0x01  | ?           |
| 1    | 0x01  | ?           |
| 2    | 0x00  | ?           |
| 3    | 0x00  | ?           |
| 4    | 0–3   | Backlight brightness |
| 5    | 0x00  | ?           |
| 6    | 0x01  | ?           |
| 7    | 1–3   | Current profile |


Mode query (bRequest = 5)
----------------------------

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


Macro bindings (bRequest = 16)
------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 16     | send macro bindings    |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      | 95     | For the Windows software, but may change |

Data sent is:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Bind count (usually 0x12 = 18 G keys) |
| 1–2  | int16 be | Size of this data |
| 3–4  | int16 be | Size of the macro data (sent with request 18) |
| 5–…  | struct array | An array of 5-byte-long bind structures |

Bind structure is filled with zeros when disabled. Valid bind are:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Data type: 0x10 = key usage code, 0x20 = macro |
| 1–2  | int16 be | Offset in raw data |
| 3–4  | int16 be | Length of data |

Data length must not exceed 128 (or the keyboard will block until reset).


Send raw data (bRequest = 18)
------------------------------

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


Profile switch (bRequest = 20)
------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 20     | switch current profile |
| wValue       | 1–3    | New current profile    |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |

G key roles (bRequest = 22)
---------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 22     | send G key roles       |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      | 37     | For the Windows software, but may change |

Data sent is:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Key count (usually 0x12 = 18 G keys) |
| 1–…  | struct array | An array of 2-byte-long key role structures |

Key role structure is:

| Byte | Type     | Content     |
| ---- | -------- | ----------- |
| 0    | byte     | Key usage |
| 1    | byte     | Playback type: 1 = play once, 2 = repeat while held, 3 = repeat until pressed again |

Each element of this array is matched with the element of the same index in the bindings array. When the key whose usage is in this array is pressed the macro from the binding array is played.

The key usage seems to be read only in the sixteen first items, key usage for a non G-key may be used there. If a usage for the key G*k* is not present, it will use the *k*th item in this list anyway, so the macro will actually be bound to two keys the one whose usage is given here and the G-key of corresponding rank.

The playback is only used by macros (0x20 binding type) not by key usage bindings (0x10).

Maximum data length is 64.


? (bRequest = 25)
-----------------

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


? (bRequest = 32)
-----------------

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


? (bRequest = 33)
-----------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x80   | vendor, host -> device |
| bRequest     | 33     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 0      |                        |

Send before the firmware blob in firmware mode.


Send Firmware data (bRequest = 35)
----------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x80   | vendor, host -> device |
| bRequest     | 35     |                        |
| wValue       |        | Offset                 |
| wIndex       | 0      |                        |
| wLength      | 512    |                        |

Corsair firmware update software send the firmware in blocks of 512 bytes starting at offset 0x8000. K90 1.36 firmware size is 27 kb, so offset goes from 0x8000 to 0xEA00 by 0x200 increments.


Light brightness (bRequest = 49)
--------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 49     | light brightness       |
| wValue       | 0–3    | New brightness level   |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |


? (bRequest = 66)
-----------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0xC0   | vendor, device -> host |
| bRequest     | 66     |                        |
| wValue       | 0x0000 |                        |
| wIndex       | 0      |                        |
| wLength      | 4      |                        |
