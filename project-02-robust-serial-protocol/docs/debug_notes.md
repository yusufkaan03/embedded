# Debug Notes

## UART RX Appeared Not to Work

### Problem

`BOOT_OK` was visible in picocom, but typed characters were not returned by STM32.

### Investigation

1. Enabled picocom local echo.
2. Performed an FT232 TX-RX loopback test.
3. Confirmed the FT232 adapter and terminal were working.
4. Inspected the UART callback and main-loop event flag.
5. Found that two different echo implementations had temporarily been mixed:
   - Echo inside the callback
   - Flag-based echo inside the main loop

### Solution

The final learning implementation was simplified so that:

- The callback only sets `uart_rx_ready`.
- The main loop sends the received byte.
- The flag is cleared after processing.
- UART reception is started again for the next byte.

### Lesson Learned

Seeing transmitted data from STM32 proves only the TX path. UART RX must be tested separately.
