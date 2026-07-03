# Known Limitations - Project 00 Startup Platform

## 1. ADC is not implemented yet

This project currently reads only the onboard user button state.

ADC reading from a potentiometer is not implemented yet. It will be added in the next step or in the sensor logger project.

## 2. Fixed serial port is used

The Python logger currently uses a fixed serial port: `/dev/ttyUSB0`.

If the FT232 adapter appears as another port, the `SERIAL_PORT` value inside `serial_logger.py` must be changed manually.

A future improvement would be adding command-line arguments such as port and baud rate selection.

Example:

`python src/serial_logger.py --port /dev/ttyUSB0 --baud 115200`

## 3. UART protocol has no checksum

The STM32 currently sends plain CSV-formatted text over UART.

Example output:

`timestamp_ms,button_state,status`

`1000,0,OK`

`2000,1,OK`

There is no checksum, packet framing, or error detection mechanism yet.

This is acceptable for Project 00, but a more robust serial protocol will be developed in a later project.

## 4. STM32 only transmits data

In this project, the STM32 only sends data to the computer.

The computer does not send commands back to the STM32 yet.

Command reception and command-response behavior will be added in a later serial protocol project.

## 5. Basic button handling

The user button is read using simple polling inside the main loop.

A small delay is used, but there is no advanced debounce algorithm or interrupt-based button handling yet.

Future improvements could include:

- Software debounce
- External interrupt for button press
- Event-based button handling

## 6. Python logger runs until manually stopped

The Python logger currently runs continuously until the user stops it with `Ctrl + C`.

Future improvements could include:

- Stop after a fixed number of samples
- Stop after a fixed duration
- Save timestamped log files automatically

## 7. No live plotting yet

The Python tool saves UART data into a CSV file, but it does not plot the data yet.

A future improvement would be adding a simple graph for button state or sensor data.

## 8. No real sensor data yet

This project is a startup platform.

It verifies GPIO input, GPIO output, UART transmission, and Python logging.

It does not read external sensors yet.

External sensor reading will be added in Project 01 - Embedded Sensor Logger.

## Summary

Project 00 successfully establishes the basic STM32-to-computer data flow.

Current working features:

- GPIO output
- GPIO input
- UART transmit
- CSV-formatted data stream
- Python serial logging
- CSV file saving

The limitations listed here are planned improvements for later projects.

