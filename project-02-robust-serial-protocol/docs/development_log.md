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
