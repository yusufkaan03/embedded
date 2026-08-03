# Interview Summary

## 30-Second Version

I built a small UART binary command protocol on an STM32F407 board. I used a state machine to parse incoming bytes. I checked the command, length, payload, and XOR checksum. I added PING, LED control, LED status, and ADC read commands. The STM32 sends binary response packets with an error code, response length, and data. I also added timeout and parser recovery.

## Technical Points

- UART interrupt reception
- Parser state machine
- XOR request and response checksum
- Command-specific validation
- Error-code based response
- High-byte and low-byte ADC transfer
- Timeout and recovery
- Debug and protocol mode separation

## Main Limitation

The current UART receive system does not use a ring buffer.  
Because of this, very fast back-to-back packets may cause byte loss.
