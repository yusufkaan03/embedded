# Known Limitations

The project passed its planned tests, but some parts are intentionally simple.

These limitations can be improved in future projects.

---

## 1. Buffer Overflow Recovery

The firmware detects buffer full and sends:

```text
0xA6
```

However, it does not:

- track the exact lost byte
- track how many bytes were lost
- find the exact stream gap
- fully resynchronize the byte stream

After a byte is lost, later packets can produce errors such as:

```text
A1 - payload length overflow
A3 - invalid command
A4 - checksum error
```

This was seen in the final stress test.

For this project, this behavior is accepted.

---

## 2. ProcessBuffer Is Blocking

`ProcessBuffer()` can wait for incoming packet data.

UART interrupts still work during this time, but the main loop cannot do other application tasks at the same time.

A future project could use a fully non-blocking parser state machine.

---

## 3. UART TX Is Blocking

The firmware uses:

```text
HAL_UART_Transmit(..., HAL_MAX_DELAY)
```

A future version could use:

- UART TX interrupt
- DMA
- TX ring buffer

---

## 4. ADC Reading Is Blocking

The ADC command uses polling.

The firmware tries 10 reads before returning the result.

This blocks the main application for a short time.

---

## 5. XOR Checksum

The protocol uses a simple XOR checksum.

It is easy to understand and enough for this learning project.

A stronger future protocol could use:

```text
CRC-8
CRC-16
```

---

## 6. Fixed Payload Size

Maximum payload:

```text
3 bytes
```

This is enough for the current commands, but larger future commands would require a bigger design.

---

## 7. Fixed Ring Buffer Size

Ring buffer:

```text
50 bytes
```

The size is fixed at compile time.

---

## 8. No Advanced Framing

The protocol uses:

```text
0xAA
```

as the start byte.

There is no:

- byte stuffing
- escaping
- COBS
- SLIP
- multi-byte magic sequence

Normal packets work because packet length is known.

Advanced recovery after corrupted streams is not implemented.

---

## 9. Most Code Is in main.c

The project still keeps most logic inside `main.c`.

A future project could separate:

```text
protocol.c
uart_transport.c
commands.c
app.c
```

This refactor was not added because the project had already become long.

---

## 10. Command IDs Must Stay Consecutive

Command validation uses `COMMAND_COUNT`.

The current command IDs are:

```text
01
02
03
04
```

If future command IDs are not consecutive, command validation should be changed.

---

## 11. Some Errors Were Not Directly Forced

The Python test did not directly force:

```text
A2
A5
C0
C1
```

These errors need internal inconsistency or hardware failure.

They are kept as defensive error paths.
