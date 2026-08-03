# Known Limitations

1. UART receives one byte at a time.
2. UART reception is not restarted immediately inside the callback.
3. There is no ring buffer.
4. Fast back-to-back packets may cause byte loss.
5. UART transmit is blocking.
6. ADC reading is blocking.
7. Debug code and command code are not fully separated.
8. Some debug paths may run the LED operation again.
9. Most of the code is inside `main.c`.
10. There is no packet sequence number.
11. There is no ACK or retry system.
12. There is no automatic Python test tool yet.

These limitations are accepted for Practice 06.  
Practice 07 will improve some of these points.
