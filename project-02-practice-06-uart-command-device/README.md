# Project 02 – Practice 06
## UART Binary Command Device

> **Goal:** Build a small UART command system on the STM32F407G-DISC1.  
> The system receives binary packets, checks them, runs commands, and sends binary response packets.

---

## 1. Project Summary

This project combines the UART topics learned in the earlier practice projects.

The system can:

1. Receive UART data byte by byte.
2. Wait for the start byte `0xAA`.
3. Read the command, length, payload, and checksum.
4. Check whether the packet is valid.
5. Run the requested command.
6. Send a binary response packet.
7. Detect incomplete packets with a timeout.
8. Work in debug mode or protocol mode.

This project is more than a simple UART print example.  
It is a small communication protocol between a computer and an STM32 board.

---

## 2. Hardware

- STM32F407G-DISC1
- Green LED on the board
- Potentiometer or another analog input source
- USB-UART connection
- Computer
- CuteCom or another serial terminal that can send binary/hex data

---

## 3. Software and Tools

- STM32CubeIDE
- STM32 HAL
- C language
- UART interrupt reception
- ADC polling
- CuteCom
- Git and GitHub

UART settings:

```text
Baud rate : 115200
Data bits : 8
Parity    : None
Stop bits : 1
```

---

## 4. System Flow

```text
Computer
   |
   | Binary request packet
   v
UART interrupt
   |
   v
Receive one byte
   |
   v
Parser state machine
   |
   +--> Check packet structure
   +--> Check checksum
   +--> Check command
   +--> Check length
   +--> Check payload
   |
   v
Run command
   |
   v
Create binary response packet
   |
   v
Computer
```

Parser states:

```text
PARSER_WAIT_START
PARSER_READ_COMMAND
PARSER_READ_LENGTH
PARSER_READ_PAYLOAD
PARSER_READ_CALC_CHECKSUM
PARSER_HANDLER
PARSER_ERROR
```

---

## 5. Request Packet Format

The computer sends this packet to the STM32:

```text
| START | COMMAND | LENGTH | PAYLOAD... | CHECKSUM |
```

| Field | Size | Description |
|---|---:|---|
| START | 1 byte | Always `0xAA` |
| COMMAND | 1 byte | Command ID |
| LENGTH | 1 byte | Number of payload bytes |
| PAYLOAD | 0–3 bytes | Command data |
| CHECKSUM | 1 byte | XOR checksum |

Request checksum:

```text
CHECKSUM = COMMAND XOR LENGTH XOR PAYLOAD[0] XOR ...
```

The start byte is not included in the checksum.

Example PING request:

```text
AA 01 00 01
```

Checksum:

```text
0x01 XOR 0x00 = 0x01
```

---

## 6. Response Packet Format

The STM32 sends this packet to the computer:

```text
| START | COMMAND | ERROR_STATUS | RESPONSE_LENGTH | DATA... | CHECKSUM |
```

| Field | Size | Description |
|---|---:|---|
| START | 1 byte | Always `0xAA` |
| COMMAND | 1 byte | Command ID from the request |
| ERROR_STATUS | 1 byte | `0x00` means success |
| RESPONSE_LENGTH | 1 byte | Number of data bytes |
| DATA | 0–3 bytes | Command result |
| CHECKSUM | 1 byte | XOR checksum |

Response checksum:

```text
CHECKSUM =
COMMAND XOR ERROR_STATUS XOR RESPONSE_LENGTH XOR DATA[0] XOR ...
```

The start byte is not included.

---

## 7. Commands

### 7.1 PING – `0x01`

Checks whether the device and protocol are working.

Request:

```text
AA 01 00 01
```

Response:

```text
AA 01 00 01 01 01
```

Response data `0x01` means PONG.

---

### 7.2 SET_LED – `0x02`

Turns the green LED on or off.

Payload:

```text
0x00 → LED OFF
0x01 → LED ON
```

LED ON request:

```text
AA 02 01 01 02
```

LED ON response:

```text
AA 02 00 01 01 02
```

LED OFF request:

```text
AA 02 01 00 03
```

LED OFF response:

```text
AA 02 00 01 00 03
```

---

### 7.3 GET_LED – `0x03`

Reads the current LED state.

Request:

```text
AA 03 00 03
```

LED OFF response:

```text
AA 03 00 01 00 02
```

LED ON response:

```text
AA 03 00 01 01 03
```

---

### 7.4 GET_ADC – `0x04`

Reads the ADC value.

The system takes 10 ADC samples and calculates the average.

Request:

```text
AA 04 00 04
```

The response has two data bytes:

```text
DATA[0] = High byte
DATA[1] = Low byte
```

The computer can rebuild the ADC value like this:

```text
adc_raw = (DATA[0] << 8) | DATA[1]
```

Example:

```text
ADC value = 4095 = 0x0FFF
```

Response:

```text
AA 04 00 02 0F FF F6
```

---

## 8. Error Codes

| Code | Error |
|---:|---|
| `0x00` | NO_ERROR |
| `0x01` | INVALID_COMMAND_ERROR |
| `0x02` | LENGTH_OVERFLOW_ERROR |
| `0x03` | INVALID_LENGTH_ERROR |
| `0x04` | INVALID_PAYLOAD_ERROR |
| `0x05` | CHECKSUM_ERROR |
| `0x06` | ADC_READ_ERROR |
| `0x07` | TIMEOUT_ERROR |
| `0x08` | SET_LED_WRITE_ERROR |

For an error response:

```text
ERROR_STATUS    = error code
RESPONSE_LENGTH = 0x00
DATA            = none
```

Example invalid command:

```text
Request : AA 55 00 55
Response: AA 55 01 00 54
```

Example invalid LED payload:

```text
Request : AA 02 01 03 00
Response: AA 02 04 00 06
```

---

## 9. Validation Order

The system checks the packet in this order:

```text
1. Is the payload length larger than the buffer?
2. Is the checksum correct?
3. Is the command supported?
4. Is the length correct for this command?
5. Is the payload value valid?
6. Can the command run correctly?
7. Create the response packet.
```

This order prevents invalid packets from running commands.

---

## 10. Timeout

The parser starts a timer after it receives the start byte.

```text
TIMEOUT_TIME = 5000 ms
```

If the packet is not completed in 5 seconds, the system returns:

```text
ERROR_STATUS = TIMEOUT_ERROR
```

Example: only `AA` is sent:

```text
AA 00 07 00 07
```

Example: `AA 01` is sent:

```text
AA 01 07 00 06
```

After timeout, the parser must reset and accept a new packet.

---

## 11. Operating Modes

The code has two operating modes:

```c
#define OPERATING_MODE 0
```

### Protocol Mode – `0`

```text
OPERATING_MODE = 0
```

Only raw binary response packets should be sent.

Expected:

```text
AA 01 00 01 01 01
```

Not expected:

```text
BOOT_OK
RESULT:OK
RESPONSE:PONG
```

### Debug Mode – `1`

```text
OPERATING_MODE = 1
```

This mode shows readable messages:

- Received command
- Received length
- Payload
- Checksum
- Success result
- Error name
- Command result

Debug mode is useful during development.  
Protocol mode is used when a computer program reads the packets.

---

## 12. Test Summary

The following functions were tested:

- PING
- LED ON
- LED OFF
- GET_LED
- GET_ADC
- Invalid payload
- Invalid command
- Invalid length
- Wrong checksum
- Length overflow
- Timeout
- Recovery after error
- Recovery after timeout
- Response checksum
- ADC high-byte and low-byte transfer

More details:

```text
docs/protocol.md
docs/test_results.md
```

---

## 13. Known Limitations

This project works, but it is not a production-level system.

Main limitations:

1. UART receives only one byte at a time.
2. UART receive is not restarted immediately inside the callback.
3. There is no ring buffer.
4. Fast packets may cause byte loss.
5. UART transmit is blocking.
6. ADC reading is blocking.
7. Debug code and command code are not fully separated.
8. Some debug paths can run the LED operation again.
9. Most code is inside `main.c`.
10. There is no automatic Python test tool.
11. There is no packet sequence number.
12. There is no retry system.

These points can be improved in Practice 07.

---

## 14. What I Learned

In this project, I learned:

- UART interrupt reception
- State machine design
- Binary packet design
- Request and response packet difference
- XOR checksum
- Command IDs
- Length validation
- Payload validation
- Error codes
- Timeout and recovery
- Splitting a 16-bit ADC value into two bytes
- Difference between ASCII text and raw binary data
- Debug mode and protocol mode
- Manual protocol testing

Important lesson:

> Sending data over UART is not the same as building a safe and testable communication protocol.

---

## 15. Completion Checklist

- [x] Request parser works
- [x] XOR checksum works
- [x] PING works
- [x] SET_LED works
- [x] GET_LED works
- [x] GET_ADC works
- [x] Binary response packets work
- [x] Error codes are sent
- [x] ADC is sent as two bytes
- [x] Timeout works
- [x] Parser recovers after errors
- [x] Debug and protocol modes are separated
- [x] Manual tests are completed
- [x] Documentation is prepared

Practice 06 is complete after the final protocol-mode checks are confirmed.

---

## 16. Practice 07 Goal

Practice 07 will rebuild the system from the beginning.

Planned improvements:

- UART RX ring buffer
- Fast receive restart in callback
- Back-to-back packet support
- Better file structure
- `protocol.c/.h`
- `commands.c/.h`
- Separate transport and application layers
- Python automatic test tool
- Automatic PASS/FAIL results
- Stress tests
- Recovery tests

Practice 06 should stay as a reference.  
Practice 07 should be written again without directly copying the old code.

---

## 17. Short Interview Explanation

> I built a small UART binary command protocol on an STM32F407 board. I used a state machine to parse incoming bytes. I checked the command, length, payload, and XOR checksum. I added PING, LED control, LED status, and ADC read commands. The device sends binary response packets with an error code, response length, and data. I also added timeout and parser recovery. The current limitation is that the UART receive system does not use a ring buffer, so very fast packets may cause byte loss.
