# Test Results

## TEST-001 — Boot Message

**Action:** Reset the STM32 board.

**Expected result:** Terminal displays `BOOT_OK`.

**Result:** PASS

---

## TEST-002 — FT232 Loopback

**Action:** Connect FT232 TXD directly to RXD and type characters.

**Expected result:** Characters return to picocom.

**Result:** PASS

---

## TEST-003 — UART Interrupt Echo

**Action:** Open picocom with local echo disabled and type characters.

**Expected result:** Characters appear only after STM32 receives and retransmits them.

**Result:** PASS

---

## TEST-004 — Multiple Characters

**Input:** `ABC123`

**Expected result:** `ABC123`

**Result:** PASS

## Text Command Handler Tests

| Test | Input | Expected | Result |
|---|---|---|---|
| PING command | `PING` | `PONG` | PASS |
| Version query | `GET_VERSION` | `VERSION:1.0.0` | PASS |
| Unknown command | `HELLO` | `ERR:UNKNOWN_COMMAND` | PASS |
| Case sensitivity | `ping` | `ERR:UNKNOWN_COMMAND` | PASS |
| Buffer overflow | More than 31 characters | `ERR:RX_OVERFLOW` | PASS |
| Overflow recovery | `PING` after overflow | `PONG` | PASS |

## LED Command Tests

| Test | Input | Expected | Result |
|---|---|---|---|
| Initial LED state | `GET_LED` | `LED:0` | PASS |
| Turn LED on | `SET_LED 1` | `OK:LED_ON` | PASS |
| Read LED on state | `GET_LED` | `LED:1` | PASS |
| Turn LED off | `SET_LED 0` | `OK:LED_OFF` | PASS |
| Read LED off state | `GET_LED` | `LED:0` | PASS |
| Invalid value | `SET_LED 2` | `ERR:INVALID_PAYLOAD` | PASS |
| Missing payload | `SET_LED` | `ERR:INVALID_PAYLOAD` | PASS |
| Extra payload | `SET_LED 1 EXTRA` | `ERR:INVALID_PAYLOAD` | PASS |
| Unknown command | `LED_ON` | `ERR:UNKNOWN_COMMAND` | PASS |

## Parser State Tests

| Test | Expected Result | Result |
|---|---|---|
| Ignore non-start byte | Remain in `PARSER_WAIT_START` | PASS |
| Detect `0xAA` | Move to `PARSER_READ_COMMAND` | PASS |
| Store command byte | Move to `PARSER_READ_LENGTH` | PASS |
| Zero payload length | Move to `PARSER_READ_CHECKSUM` | PASS |
| Valid non-zero length | Move to `PARSER_READ_PAYLOAD` | PASS |
| Length above 16 | Reset to `PARSER_WAIT_START` | PASS |

## Binary Payload Parser Tests

| Test | Input | Expected Result | Result |
|---|---|---|---|
| One-byte payload | `AA 03 01 01` | Store `01` and move to checksum state | PASS |
| First byte of three-byte payload | `01` | Index becomes 1, remain in payload state | PASS |
| Second byte of three-byte payload | `00` | Index becomes 2, remain in payload state | PASS |
| Third byte of three-byte payload | `01` | Index becomes 3, move to checksum state | PASS |
| Payload order | `01 00 01` | Buffer contains `01 00 01` | PASS |
| Text protocol regression | `PING` | `PONG` | PASS |

## Binary Checksum Validation Tests

| Test | Packet | Expected Result | Result |
|---|---|---|---|
| Valid SET_LED packet | `AA 03 01 01 03` | Packet valid | PASS |
| Invalid SET_LED checksum | `AA 03 01 01 00` | Packet invalid | PASS |
| Valid PING without payload | `AA 01 00 01` | Packet valid | PASS |
| Parser returns to start state | Complete packet | `PARSER_WAIT_START` | PASS |
| Text protocol regression | `PING` | `PONG` | PASS |
