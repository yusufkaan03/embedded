# Project 02 – Practice 08
## Robust UART Protocol with Ring Buffer

## Overview

This project is the final practice of Project 02.

The goal was to build a more reliable binary UART protocol on the STM32F407G-DISC1.

The biggest improvement is the RX system:

```text
UART RX Interrupt
       |
       v
Circular Ring Buffer
       |
       v
Packet Processing
       |
       v
Parser
       |
       v
Command Execution
       |
       v
Binary Response
```

The UART interrupt receives and stores bytes quickly.  
The main program reads and processes these bytes later.

This means UART reception and packet processing are separated.

---

## Main Features

- Binary UART protocol
- UART RX interrupt
- 50-byte circular ring buffer
- Separate read and write indexes
- Ring buffer wrap handling
- Buffer full detection
- Back-to-back packet support
- Length-based packet reading
- 5-second inter-byte timeout
- XOR checksum
- Command validation
- Payload validation
- Error codes
- LED control
- ADC reading
- Python automated tests

---

## Request Packet

```text
START | COMMAND | LENGTH | PAYLOAD... | CHECKSUM
```

Start byte:

```text
0xAA
```

Checksum:

```text
COMMAND XOR LENGTH XOR PAYLOAD bytes
```

The start byte is not included in the checksum.

Maximum payload size:

```text
3 bytes
```

---

## Response Packet

```text
START | COMMAND | ERROR | LENGTH | DATA... | CHECKSUM
```

Response checksum:

```text
COMMAND XOR ERROR XOR LENGTH XOR DATA bytes
```

---

## Commands

| Command | ID | Request | Response |
|---|---:|---|---|
| PING | `0x01` | no payload | `0x01` |
| SET_LED | `0x02` | `0x00` or `0x01` | LED state |
| GET_LED | `0x03` | no payload | LED state |
| GET_ADC | `0x04` | no payload | 2-byte ADC value |

ADC response:

```text
HIGH_BYTE | LOW_BYTE
```

The firmware tries 10 ADC conversions.  
At least 3 successful conversions are required.

---

## Error Codes

Errors are grouped by type.

```text
0xA* -> RX / protocol errors
0xB* -> command errors
0xC* -> execution errors
```

### RX / Protocol

| Code | Error |
|---:|---|
| `0xA0` | RX timeout |
| `0xA1` | RX payload length overflow |
| `0xA2` | Start byte check failed |
| `0xA3` | Invalid command |
| `0xA4` | Checksum error |
| `0xA5` | Packet inconsistency |
| `0xA6` | RX buffer full |

### Command

| Code | Error |
|---:|---|
| `0xB0` | Invalid payload length |
| `0xB1` | Invalid payload value |

### Execution

| Code | Error |
|---:|---|
| `0xC0` | LED verification failed |
| `0xC1` | ADC read failed |

---

## Ring Buffer

The ring buffer size is:

```text
50 bytes
```

The UART interrupt controls the write side.

The main program controls the read side.

The indexes return to zero when they reach the end of the buffer.

The project also keeps separate wrap information for read and write positions.

This allows the firmware to understand the difference between:

```text
same index + same wrap state      -> buffer empty
same index + different wrap state -> buffer full
```

When the buffer is full, new data is not written over unread data.

The firmware sets an overflow flag and later sends:

```text
0xA6 - RX BUFFER FULL
```

The project only reports this condition.  
It does not try to recover every lost byte.

---

## Timeout

Timeout value:

```text
5000 ms
```

The timeout is used only when a packet starts but does not finish.

Each new packet byte refreshes the timeout.

A complete packet is processed immediately.

---

## Python Test Tool

Folder structure:

```text
python_tools/
├── data/
└── src/
    └── practice08_uart_final_test.py
```

Run example:

```bash
python3 python_tools/src/practice08_uart_final_test.py     --port /dev/ttyUSB0     --baud 115200
```

The Python tool saves `.md` and `.log` results inside:

```text
python_tools/data/
```

---

## Final Automated Test

Final result:

```text
PASS : 17
FAIL : 0
WARN : 0

RESULT: CORE TEST SUITE PASSED
```

Important tests:

```text
Back-to-back:
6 requests -> 6 valid responses

Ring buffer wrap:
40 PING responses correct
160 request bytes processed

Overflow stress:
200 PING packets sent in one burst
A6 buffer-full notifications detected
0 bad response checksums
```

The overflow test also produced some `A1`, `A3`, and `A4` errors after bytes were lost.

This is expected for the current project because advanced stream recovery is not implemented.

---

## Documentation

More details are available in:

```text
docs/project_summary.md
docs/known_limitations.md
docs/debug_notes.md
```

---

## Project Status

```text
Firmware: COMPLETE
Python tests: COMPLETE
Documentation: COMPLETE
```

This project is finished within its planned scope.
