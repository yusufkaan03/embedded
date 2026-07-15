# Development Log

## Stage 1 — UART Bring-Up

- Configured USART2 in asynchronous mode.
- Used PA2 as USART2_TX and PA3 as USART2_RX.
- Configured UART as 115200 baud, 8 data bits, no parity and 1 stop bit.
- Enabled USART2 global interrupt.
- Sent a `BOOT_OK` message after startup.
- Verified STM32-to-PC communication using FT232 and picocom.

## Stage 2 — One-Byte Interrupt Receive

- Started one-byte UART reception with `HAL_UART_Receive_IT()`.
- Used `HAL_UART_RxCpltCallback()` to detect completed reception.
- Used a volatile flag to transfer the event from interrupt context to the main loop.
- Echoed received bytes back to the terminal.
- Re-armed UART reception after every received byte.

## What I Learned

- Difference between blocking transmit and interrupt-based receive.
- UART handle usage.
- Callback functions.
- Interrupt and main-loop task separation.
- Why interrupt reception must be re-armed.
- Difference between terminal local echo and device echo.

## Stage 4 — Parameterized LED Commands

- Added `SET_LED 1` and `SET_LED 0` commands.
- Added the `GET_LED` status query.
- Separated the command prefix from its payload.
- Added invalid payload handling.
- Controlled the onboard green LED through the serial protocol.

### New Concepts

- Prefix comparison with `strncmp()`
- Payload extraction using pointer arithmetic
- Application state tracking
- Command validation

## Stage 9 — Binary Payload Collection

- Added a binary payload buffer.
- Implemented the `PARSER_READ_PAYLOAD` state.
- Stored incoming payload bytes sequentially.
- Used `parser_payload_index` to track the number of received payload bytes.
- Transitioned to `PARSER_READ_CHECKSUM` after receiving the number of bytes specified by `LENGTH`.
- Verified one-byte and three-byte payload handling.

### Parser Flow

```text
START
→ COMMAND
→ LENGTH
→ PAYLOAD[0]
→ PAYLOAD[1]
→ ...
→ CHECKSUM
```

### Important Detail

The binary payload buffer is not a C string. Therefore, no null terminator (`'\0'`) is required. Only the first `parser_length` bytes are considered valid.

## Stage 10 — Binary Checksum Validation

- Implemented the `PARSER_READ_CHECKSUM` state.
- Stored the checksum received from the packet.
- Calculated the expected checksum from command, length and payload.
- Compared the received and calculated checksum values.
- Added packet-complete and packet-valid flags.
- Reset the parser to `PARSER_WAIT_START` after packet completion.
- Verified packets both with and without payload.

### Validation Flow

```text
Receive checksum byte
→ Calculate checksum locally
→ Compare received and calculated values
→ Mark packet as complete
→ Mark packet as valid or invalid
→ Return to WAIT_START
```

### Current Limitation

The binary parser is currently tested using internal self-tests.
It is not yet connected to the real UART receive stream.

## Stage 12 — Real UART Binary Parser Integration

- Connected the binary state-machine parser to the USART2 receive callback.
- Added automatic switching between text and binary receive modes.
- Entered binary mode when the start byte `0xAA` was received.
- Routed all following packet bytes to the binary parser.
- Returned to text mode after packet completion.
- Published valid packets to the main loop.
- Reported invalid checksum packets without executing them.
- Preserved the existing text command interface.

### Receive Flow

```text
UART byte received
        |
        +-- Byte is 0xAA or binary mode is active
        |       |
        |       +--> Binary parser
        |
        +-- Otherwise
                |
                +--> Text line buffer
```

### Current Binary Responses

- Valid binary PING: `BIN:PONG`
- Valid but unsupported binary packet: `BIN:PACKET_OK`
- Invalid checksum: `BIN:BAD_CHECKSUM`

### Current Limitation

Binary requests are parsed from real UART data, but responses are still sent as readable text. Binary response packet generation has not yet been implemented.
