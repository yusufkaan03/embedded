# Binary Packet Parser Simulator

## Overview

This project is a simple binary packet parser simulator for STM32.

The parser reads predefined byte arrays one byte at a time and follows this packet format:

```text
START | COMMAND | LENGTH | PAYLOAD | CHECKSUM
```

The main goal was to understand how a state machine can separate and validate packet fields.

## Packet Format

| Field | Description |
|---|---|
| `START` | Fixed start byte: `0xAA` |
| `COMMAND` | Command value |
| `LENGTH` | Number of payload bytes |
| `PAYLOAD` | Data bytes, maximum 3 bytes |
| `CHECKSUM` | XOR checksum |

Checksum calculation:

```text
COMMAND XOR LENGTH XOR PAYLOAD bytes
```

## What Was Implemented

- Start byte detection
- Command and length reading
- Payload collection
- Zero-length payload support
- XOR checksum validation
- Length overflow detection
- Noise byte rejection
- Recovery after invalid packets
- Consecutive packet processing

## Parser States

```text
PARSER_WAIT_START
PARSER_READ_COMMAND
PARSER_READ_LENGTH
PARSER_READ_PAYLOAD
PARSER_READ_CHECKSUM
PARSER_CALCULATE_CHECKSUM
PARSER_ERROR
```

## Test Summary

The parser was tested with:

- Valid packets
- Invalid checksum values
- Payload lengths from 0 to 3
- Length overflow
- Noise bytes between packets
- Multiple packets in one byte stream

All planned tests passed.

Detailed test outputs are available in:

```text
docs/test_results.md
```

## Current Limitations

- Input comes from predefined arrays.
- Live UART reception is not implemented yet.
- Parsed commands are not executed.
- Packet timeout is not implemented.

## What I Learned

- How a byte-by-byte parser works
- How a state machine controls packet flow
- How payload length protects the buffer
- How XOR checksum validation works
- How to recover after invalid data
- How to parse more than one packet in the same data stream
