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
