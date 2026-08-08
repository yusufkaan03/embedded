# Debug Notes

This file records the main problems found during the project and how they were handled.

---

## 1. UART Reception Was Restarted Too Late

### Problem

The first design received a byte, processed it in main code, and then restarted UART reception.

This could lose new bytes while the main code was busy.

### Change

The final design restarts UART reception inside the RX callback.

The callback stores the byte in the ring buffer and exits quickly.

### Lesson

```text
Receive first.
Store quickly.
Process later.
```

---

## 2. Timeout Was Used as Packet End

### Problem

An older design waited for silence before deciding that a packet was complete.

This created a large delay.

It also made back-to-back packets difficult.

### Change

The packet length byte is now used to know the expected packet size.

```text
total request size = payload length + 4
```

The 5-second timeout is only used when a packet starts but does not finish.

---

## 3. Ring Buffer Needed Read and Write Positions

### Problem

Using only one buffer index was not enough.

The system needed to know:

```text
where new bytes are written
where old bytes are read
```

### Change

Separate indexes were created:

```text
buffer_write_index
buffer_read_index
```

---

## 4. Full and Empty Looked the Same

### Problem

In a circular buffer:

```text
read_index == write_index
```

can mean:

```text
buffer empty
```

or:

```text
buffer full
```

### Change

Separate wrap flags were added.

```text
same index + same wrap state
-> empty

same index + different wrap state
-> full
```

---

## 5. Buffer Data Was Originally Moved

### Problem

An early version moved remaining bytes to the start of the array after reading data.

This was unnecessary and could conflict with interrupt writes.

### Change

The final design does not move the data.

Only the read and write indexes move.

---

## 6. Payload Validation Logic

### Problem

The first range check used the wrong Boolean condition.

A value could pass even when it was outside the allowed range.

### Change

A payload is now invalid when:

```text
value < minimum
OR
value > maximum
```

For `SET_LED`:

```text
0 -> valid
1 -> valid
other values -> B1 error
```

---

## 7. COMMAND_WAIT Was Accepted as a Command

### Problem

Command value `0x00` was originally accepted even though it was only an internal wait state.

### Change

`0x00` is now rejected as an invalid command.

---

## 8. Buffer Full and Overflow

### Problem

When the write index caught the read index, new bytes could overwrite unread data.

### Change

The firmware now checks the full state before writing.

When full:

```text
do not overwrite unread data
set buffer-full flag
continue UART reception
report A6 later
```

### Project Decision

The firmware does not try to recover the exact lost byte.

This was intentionally left as a future topic.

---

## 9. Back-to-Back Packets

The final automated test sent multiple commands without manual delay.

Result:

```text
6 requests -> 6 valid responses
```

This confirmed that the ring-buffer design solved the main limitation of the earlier UART design.

---

## 10. Ring Wrap Test

The final test processed:

```text
40 PING requests
160 request bytes
```

The ring buffer size is only:

```text
50 bytes
```

So the read and write indexes passed the end of the array several times.

All 40 responses were correct.

---

## 11. Overflow Stress Test

The test sent:

```text
200 PING packets
```

in one burst.

The firmware detected buffer-full conditions.

Result included:

```text
A6 buffer-full notifications
0 bad response checksums
```

Some later packets produced `A1`, `A3`, and `A4` because bytes were dropped and advanced stream recovery is not implemented.

This is documented as a known limitation.

---

## 12. Final Automated Result

```text
PASS : 17
FAIL : 0
WARN : 0

RESULT: CORE TEST SUITE PASSED
```

At this point the project was closed instead of adding more protocol features.
