## UART log timing limitation

The STM32 sends UART log data periodically.

In this project, the UART log is transmitted once every 1 second. Because of this, the logger records only the current system state at the moment of transmission.

If the user button is pressed multiple times between two UART transmissions, some intermediate mode changes may not appear in the CSV log.

Example:

```text
button_count: 4 -> 5 -> 7
```

In this case, `button_count = 6` may not be visible in the CSV file because the mode changed between two UART log messages.

This is not a bug in the Python logger. It is a limitation of periodic UART logging.

A future improvement could be event-based UART logging, where the STM32 sends an extra message immediately when the button is pressed.

