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
