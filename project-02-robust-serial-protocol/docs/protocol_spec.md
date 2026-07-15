# Binary Serial Protocol Specification

## Packet Format

| Field | Size | Description |
|---|---:|---|
| START | 1 byte | Packet start marker, always `0xAA` |
| COMMAND | 1 byte | Command or response identifier |
| LENGTH | 1 byte | Number of payload bytes |
| PAYLOAD | 0–16 bytes | Command-specific data |
| CHECKSUM | 1 byte | XOR of COMMAND, LENGTH and PAYLOAD |

## Checksum

The checksum is calculated as:

```text
CHECKSUM = COMMAND XOR LENGTH XOR PAYLOAD[0] ... PAYLOAD[n]
```

The START byte is not included in the checksum.

## Request Commands

|     ID | Command       | Payload              |
| -----: | ------------- | -------------------- |
| `0x01` | PING          | None                 |
| `0x02` | GET_VERSION   | None                 |
| `0x03` | SET_LED       | One byte: `0` or `1` |
| `0x04` | GET_LED       | None                 |
| `0x05` | RESET_COUNTER | None                 |

## Response Commands

|     ID | Response      |
| -----: | ------------- |
| `0x81` | PONG          |
| `0x82` | VERSION       |
| `0x83` | LED_SET       |
| `0x84` | LED_STATE     |
| `0x85` | COUNTER_RESET |
| `0xFF` | ERROR         |

## Error Codes

|   Code | Error           |
| -----: | --------------- |
| `0x01` | UNKNOWN_COMMAND |
| `0x02` | INVALID_LENGTH  |
| `0x03` | INVALID_PAYLOAD |
| `0x04` | BAD_CHECKSUM    |
| `0x05` | RX_TIMEOUT      |
| `0x06` | BUFFER_OVERFLOW |

## Example — SET_LED ON

AA 03 01 01 03

| Byte | Meaning        |
| ---: | -------------- |
| `AA` | START          |
| `03` | SET_LED        |
| `01` | Payload length |
| `01` | LED ON         |
| `03` | Checksum       |

