# Protocol Reference

## Request Packet

```text
| 0xAA | COMMAND | LENGTH | PAYLOAD... | CHECKSUM |
```

```text
request_checksum = COMMAND XOR LENGTH XOR every payload byte
```

## Response Packet

```text
| 0xAA | COMMAND | ERROR_STATUS | RESPONSE_LENGTH | DATA... | CHECKSUM |
```

```text
response_checksum =
COMMAND XOR ERROR_STATUS XOR RESPONSE_LENGTH XOR every data byte
```

## Command Table

| Command | ID | Request Length | Response Length |
|---|---:|---:|---:|
| PING | `0x01` | 0 | 1 |
| SET_LED | `0x02` | 1 | 1 |
| GET_LED | `0x03` | 0 | 1 |
| GET_ADC | `0x04` | 0 | 2 |

## Error Table

| Error | Code |
|---|---:|
| NO_ERROR | `0x00` |
| INVALID_COMMAND_ERROR | `0x01` |
| LENGTH_OVERFLOW_ERROR | `0x02` |
| INVALID_LENGTH_ERROR | `0x03` |
| INVALID_PAYLOAD_ERROR | `0x04` |
| CHECKSUM_ERROR | `0x05` |
| ADC_READ_ERROR | `0x06` |
| TIMEOUT_ERROR | `0x07` |
| SET_LED_WRITE_ERROR | `0x08` |

## ADC Byte Order

GET_ADC uses high byte first:

```text
DATA[0] = high byte
DATA[1] = low byte
```

Rebuild the value:

```text
adc_raw = (DATA[0] << 8) | DATA[1]
```

## Examples

```text
PING request       : AA 01 00 01
PING response      : AA 01 00 01 01 01

LED ON request     : AA 02 01 01 02
LED ON response    : AA 02 00 01 01 02

LED OFF request    : AA 02 01 00 03
LED OFF response   : AA 02 00 01 00 03

GET_LED request    : AA 03 00 03
GET_LED OFF        : AA 03 00 01 00 02
GET_LED ON         : AA 03 00 01 01 03

GET_ADC request    : AA 04 00 04
ADC 4095 response  : AA 04 00 02 0F FF F6

Invalid payload    : AA 02 01 03 00
Error response     : AA 02 04 00 06

Invalid command    : AA 55 00 55
Error response     : AA 55 01 00 54
```
