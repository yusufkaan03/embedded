# Test Results

## Test Environment

- Target: STM32F407G-DISC1
- Input source: predefined byte arrays
- Output: serial terminal
- Start byte: `0xAA`
- Maximum payload length: 3 bytes
- Checksum method: XOR of command, length, and payload bytes

## Result Summary

| ID | Test | Expected Result | Status |
|---|---|---|---|
| T01 | Length overflow | Reject packet and return to start search | PASS |
| T02 | Valid one-byte payload | Parse packet and validate checksum | PASS |
| T03 | Invalid packet followed by valid packet | Recover and parse second packet | PASS |
| T04 | Invalid packet, noise, then valid packet | Ignore noise and parse valid packet | PASS |
| T05 | Two consecutive valid packets | Parse both packets independently | PASS |
| T06 | Invalid checksum followed by valid packet | Reject first and accept second | PASS |

---

## T01 — Length Overflow

### Input

```text
AA 03 04
```

### Purpose

Verify that a payload length larger than the three-byte payload buffer is rejected.

### Expected Result

- Start byte is detected.
- Command `0x03` is stored.
- Length `0x04` is rejected.
- No payload byte is written.
- The parser reports `LENGTH_OVERFLOW`.
- The parser returns to the start-search state.

### Terminal Output

```text
FIRST_PACKET: AA 03 04
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
ERROR_STATE_3:PASS VALUE: LENGTH_OVERFLOW
```

### Result

**PASS**

---

## T02 — Valid Packet with One Payload Byte

### Input

```text
AA 03 01 01 03
```

### Checksum

```text
03 XOR 01 XOR 01 = 03
```

### Expected Result

- Command: `3`
- Length: `1`
- Payload: `[0] = 1`
- Received checksum: `3`
- Checksum result: `TRUE`

### Terminal Output

```text
SECOND_PACKET: AA 03 01 01 03
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:1
PARSER_STATE_4:PASS VALUE:[0]:1
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE
```

### Result

**PASS**

---

## T03 — Recovery After Length Overflow

### Input

```text
AA 03 04  AA 03 01 01 03
```

### Purpose

Verify that a valid packet can be parsed after an invalid packet in the same byte stream.

### Expected Result

- First packet produces `LENGTH_OVERFLOW`.
- Parser returns to the start-search state.
- Second `0xAA` starts a new packet.
- Second packet checksum result is `TRUE`.

### Terminal Output

```text
THIRD_PACKET: AA 03 04  AA 03 01 01 03
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
ERROR_STATE_3:PASS VALUE: LENGTH_OVERFLOW

PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:1
PARSER_STATE_4:PASS VALUE:[0]:1
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE
```

### Result

**PASS**

---

## T04 — Noise Rejection After an Invalid Packet

### Input

```text
AA 03 04 55 66 AA 03 01 01 03
```

### Purpose

Verify that bytes other than `0xAA` are ignored while the parser waits for a new packet.

### Expected Result

- First packet produces `LENGTH_OVERFLOW`.
- `0x55` and `0x66` are ignored.
- The next `0xAA` starts a valid packet.
- The valid packet checksum result is `TRUE`.

### Terminal Output

```text
FOURTH_PACKET: AA 03 04 55 66 AA 03 01 01 03
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
ERROR_STATE_3:PASS VALUE: LENGTH_OVERFLOW

PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:1
PARSER_STATE_4:PASS VALUE:[0]:1
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE
```

### Result

**PASS**

---

## T05 — Two Consecutive Valid Packets

### Input

```text
AA 03 01 01 03  AA 03 00 03
```

### Purpose

Verify that two valid packets are parsed independently in one continuous byte stream.

### First Packet

```text
COMMAND = 3
LENGTH = 1
PAYLOAD[0] = 1
CHECKSUM = TRUE
```

### Second Packet

```text
COMMAND = 3
LENGTH = 0
PAYLOAD = NO_PAYLOAD
CHECKSUM = TRUE
```

### Terminal Output

```text
FIFTH_PACKET: AA 03 01 01 03  AA 03 00 03
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:1
PARSER_STATE_4:PASS VALUE:[0]:1
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE

PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:0
PARSER_STATE_4:PASS VALUE:NO_PAYLOAD
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE
```

### Result

**PASS**

---

## T06 — Invalid Checksum Followed by a Valid Packet

### Input

```text
AA 03 01 01 00  AA 03 00 03
```

### Purpose

Verify that a completed packet with an invalid checksum does not prevent the next packet from being parsed.

### First Packet Checksum

Calculated checksum:

```text
03 XOR 01 XOR 01 = 03
```

Received checksum:

```text
00
```

Expected result:

```text
FALSE
```

### Second Packet Checksum

```text
03 XOR 00 = 03
```

Expected result:

```text
TRUE
```

### Terminal Output

```text
SIXTH_PACKET: AA 03 01 01 00  AA 03 00 03
PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:1
PARSER_STATE_4:PASS VALUE:[0]:1
PARSER_STATE_5:PASS VALUE:0
PARSER_STATE_6:PASS VALUE:FALSE

PARSER_STATE_1:PASS VALUE:AA
PARSER_STATE_2:PASS VALUE:3
PARSER_STATE_3:PASS VALUE:0
PARSER_STATE_4:PASS VALUE:NO_PAYLOAD
PARSER_STATE_5:PASS VALUE:3
PARSER_STATE_6:PASS VALUE:TRUE
```

### Result

**PASS**

---

## Final Assessment

The parser passed all six acceptance tests.

The completed simulator demonstrates:

- State-based byte parsing
- Payload-length validation
- Safe payload storage
- XOR checksum validation
- Error recovery
- Noise rejection
- Consecutive packet processing

The next development step is to connect the parser to live UART reception and add command execution.
