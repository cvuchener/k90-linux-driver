USB Control Messages for the K90 keyboard
=========================================

Request type use the vendor bit so it is 0x40 for out messages and 0xC0 for in messages.

Macro mode requests (bRequest = 2)
----------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 2      | macro mode operations  |
| wValue       | 0x0001 | Hardware mode          |
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


Unknown query (bRequest = 5)
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
| 0    | 0x01  | ?           |
| 1    | 0x01  | ?           |


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
| 0    | byte     | 0x20
| 1–2  | int16 be | Offset of the macro |
| 3–4  | int16 be | Length of the macro |


Raw macro data (bRequest = 18)
------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 18     | send raw macro data    |
| wValue       | 0x0000 |                        |
| wIndex       | 1–3    | Target profile         |
| wLength      |        | variable               |

Data contains the raw macro items referenced in request 16.

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
| 0    | byte     | Key usage (only G key usage works) |
| 1    | byte     | Playback type: 1 = play once, 2 = repeat while held, 3 = repeat until pressed again |

Each element of this array is matched with the element of the same index in the bindings array. When the key whose usage is in this array is pressed the macro from the binding array is played. G keys need not to be sorted but usage for other keys will not work.


Light brightness (bRequest = 49)
--------------------------------

| Field        | Value  | Description            |
| ------------ | ------ | ---------------------- |
| bRequestType | 0x40   | vendor, host -> device |
| bRequest     | 49     | light brightness       |
| wValue       | 0–3    | New brightness level   |
| wIndex       | 0x0000 |                        |
| wLength      | 0      |                        |



