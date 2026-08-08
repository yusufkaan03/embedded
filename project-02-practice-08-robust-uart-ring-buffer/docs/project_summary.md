# Project Summary

## What Was the Goal?

The goal was to improve the UART protocol from the earlier practices.

The old design could lose data because UART reception was restarted after main processing.

The new design uses:

```text
UART Interrupt
-> Ring Buffer
-> Parser
-> Command
-> Response
```

This separates receiving bytes from processing bytes.

---

## What Was Added?

### UART RX Interrupt

Every received byte is handled by the UART callback.

The callback:

```text
1. checks the ring buffer
2. stores the byte
3. moves the write index
4. restarts UART reception
5. exits
```

The callback does not run the parser, ADC, LED command, or UART transmit.

---

### Circular Ring Buffer

A 50-byte circular buffer stores RX data.

There are separate positions for:

```text
write index
read index
```

The write side is used by the interrupt.

The read side is used by the main program.

The indexes wrap back to zero at the end of the buffer.

Separate wrap flags are used to understand whether equal indexes mean empty or full.

---

### Length-Based Packet Reading

The packet format contains a length byte.

Because of this, the firmware knows when the packet is complete.

```text
packet size = payload length + 4
```

A complete packet is processed immediately.

The firmware does not use silence time as the normal end of a packet.

---

### Timeout

A 5-second timeout is used for incomplete packets.

This also makes manual testing possible.

Example:

```text
START received
-> wait for next byte

next byte received
-> restart timeout

packet complete
-> process immediately

required byte does not arrive
-> timeout error
```

---

### Parser and Commands

The parser checks:

- start byte
- command
- payload length
- payload data
- checksum
- packet consistency

Then the application executes one of these commands:

```text
PING
SET_LED
GET_LED
GET_ADC
```

---

### Error System

Error groups were separated:

```text
A -> RX / protocol
B -> command
C -> execution
```

This makes the returned error easier to understand during tests.

---

### Buffer Full Detection

The firmware can detect when the ring buffer is full.

When full:

```text
new byte is not stored
buffer-full flag is set
current packet processing continues
A6 notification is sent later
```

The project does not try to find the exact lost byte.

---

## Final Test

The Python final test checked normal packets, error packets, timeout, back-to-back packets, ring-buffer wrap, and overflow stress.

Final result:

```text
PASS : 17
FAIL : 0
WARN : 0
```

Important results:

```text
6 back-to-back requests -> 6 correct responses

40 PING responses -> correct
160 RX request bytes -> processed

200 PING stress test -> buffer-full notifications detected
bad response checksums -> 0
```

---

## Main Things Learned

The most important things learned in this project were:

- how UART interrupts work in a real project
- how the CPU returns to normal code after an interrupt
- why interrupt code should be short
- how a ring buffer works
- why read and write positions must be separate
- how buffer full and empty states can look the same
- why packet length is useful
- how timeout should be used
- how back-to-back packets can be received
- how binary protocol errors can be grouped
- how Python can automatically test firmware
- why some advanced problems should be left as known limitations

---

## Final Result

The main goal of the project was completed.

The final system can receive binary UART data with interrupts, store it in a ring buffer, process back-to-back packets, execute commands, return binary responses, and report protocol errors.
