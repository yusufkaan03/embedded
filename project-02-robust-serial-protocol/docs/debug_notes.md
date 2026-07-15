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

## Parser State Fall-Through

### Problem

Both parser stage self-tests failed after adding the length state.

### Cause

The `PARSER_READ_COMMAND` case did not contain a `break` statement.  
Therefore, the same byte was processed first as the command and then again as the payload length.

### Incorrect Flow

```text
0x03 received
→ parser_command = 0x03
→ missing break
→ parser_length = 0x03
→ parser state became PARSER_READ_PAYLOAD
```

### Solution

Added `break` after transitioning from `PARSER_READ_COMMAND` to `PARSER_READ_LENGTH`.

### Lesson Learned

C switch cases continue into the next case unless execution is stopped with `break`, `return`, or another control-flow statement. Parser self-tests detected the error before integration with real UART binary data.
