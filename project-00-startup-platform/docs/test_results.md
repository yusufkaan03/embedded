# Test Results - Project 00 Startup Platform

## Summary

This document records the test results for Project 00.

All basic platform tests were completed successfully. The STM32F407G-DISC1 board can control onboard LEDs, read the user button, transmit UART data, and send CSV-formatted data to a Python serial logger.

## Test Results

| Test ID | Test Name | Result | Notes |
|---|---|---|---|
| TEST-001 | Green LED Blink Test | PASS | The green onboard LED blinked every 500 ms. |
| TEST-002 | Four LED Sequence Test | PASS | Green, orange, red, and blue LEDs blinked in sequence. |
| TEST-003 | User Button Input Test | PASS | Pressing the user button turned the green LED on. Releasing the button turned it off. |
| TEST-004 | UART Text Transmit Test | PASS | `Hello from STM32` was received on Ubuntu using `picocom`. |
| TEST-005 | CSV UART Data Stream Test | PASS | STM32 sent timestamp, button state, and status fields in CSV format. |
| TEST-006 | Python Serial Logger Test | PASS | Python read the UART data and saved it into a CSV file. |

## Verified Output

Example UART/CSV output:

```text
timestamp_ms,button_state,status
5013,0,OK
6015,0,OK
7017,0,OK
8019,0,OK
```
