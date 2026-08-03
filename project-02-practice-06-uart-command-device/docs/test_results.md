# Test Results

## Test Setup

- Board: STM32F407G-DISC1
- Interface: UART
- Baud rate: 115200
- Terminal: CuteCom
- Protocol: Raw binary packet
- Checksum: XOR
- Test type: Manual test

## Functional Tests

| ID | Test | Request | Expected Response | Result |
|---:|---|---|---|---|
| T01 | PING | `AA 01 00 01` | `AA 01 00 01 01 01` | PASS |
| T02 | SET_LED OFF | `AA 02 01 00 03` | `AA 02 00 01 00 03` | PASS |
| T03 | GET_LED OFF | `AA 03 00 03` | `AA 03 00 01 00 02` | PASS |
| T04 | SET_LED ON | `AA 02 01 01 02` | `AA 02 00 01 01 02` | PASS |
| T05 | GET_LED ON | `AA 03 00 03` | `AA 03 00 01 01 03` | PASS |
| T06 | GET_ADC | `AA 04 00 04` | `AA 04 00 02 HIGH LOW CS` | PASS |

## Error Tests

| ID | Test | Request / Action | Expected Response | Result |
|---:|---|---|---|---|
| T07 | Invalid LED payload | `AA 02 01 03 00` | `AA 02 04 00 06` | PASS |
| T08 | Invalid command | `AA 55 00 55` | `AA 55 01 00 54` | PASS |
| T09 | Invalid PING length | `AA 01 01 00 00` | `AA 01 03 00 02` | PASS |
| T10 | Invalid GET_LED length | `AA 03 01 00 02` | `AA 03 03 00 00` | PASS |
| T11 | Invalid GET_ADC length | `AA 04 01 00 05` | `AA 04 03 00 07` | PASS |
| T12 | Wrong checksum | `AA 01 00 00` | `AA 01 05 00 04` | PASS |
| T13 | Length overflow | Send `AA 01 04` | `AA 01 02 00 03` | PASS |
| T14 | Timeout before command | Send `AA`, wait 5 s | `AA 00 07 00 07` | PASS |
| T15 | Timeout after command | Send `AA 01`, wait 5 s | `AA 01 07 00 06` | PASS |
| T16 | Recovery after error | Send an invalid packet, then PING | PING accepted | PASS |
| T17 | Recovery after timeout | Cause timeout, then send PING | PING accepted | PASS |

## ADC Example

Measured ADC value:

```text
4095 decimal = 0x0FFF
```

Response:

```text
AA 04 00 02 0F FF F6
```

Checksum:

```text
04 XOR 00 XOR 02 XOR 0F XOR FF = F6
```

Result: PASS

## Final Checks

- [x] No ASCII debug text in protocol mode
- [x] Timeout sends only one response
- [x] Parser accepts a packet after timeout
- [x] PING response has 6 bytes
- [x] ADC response has 7 bytes
