# Project 00 - Startup Platform

## Overview

This project is the initial STM32 startup platform for the embedded systems project series.

The goal is to verify the basic STM32 development workflow using STM32CubeMX, STM32CubeIDE, GPIO output, and ST-LINK programming.

## Checkpoint 1 - GPIO LED Blink

Status: Completed

The STM32F407G-DISC1 board was configured using STM32CubeMX.  
The project was opened in STM32CubeIDE.  
The green LED connected to PD12 was toggled every 500 ms.

## Hardware

- STM32F407G-DISC1

## Software

- STM32CubeMX
- STM32CubeIDE
- C

## Current Result

Green onboard LED is blinking successfully.

## Next Steps

- Test all four onboard LEDs
- Read user button state
- Add UART transmit
- Add ADC potentiometer reading
- Send CSV-formatted data over UART
- Read UART data using Python


## Checkpoint 2 - Onboard LED Secuence Test

Status: Copleted

All four onboard LEDs were configured as GPIO outputs and tested successfully.

LED pin mapping:

- PD12: Green LED
- PD13: Orange LED
- PD14: Red LED
- PD15: Blue LED

The LEDs blik in sequence with a 500 ms delay


## Checkpoint 3 - User Button Input

Status: Completed

The onboard user button was configured as a GPIO input

When the user button is pressed, the green onboard LED turns on.
When the user button in released, the green onboard LED turns off.

### GPIO Mapping

| Function | Pin | Description |
|---|---|---|
| USER_BUTTON | PA0 | Onboard user button |
| LD4_green | PD12 | Green onboard LED |
| LD3_orange | PD13 | Orange onboard LED |
| LD5_red | PD14 | Red onboard LED |
| LD6_red | PD15 | Blue onboard LED |

### Behavior

```text
Button pressed  -> Green LED ON
Button released -> Green LED OFF
```

## Checkpoint 4 - UART Transmit

Status: Completed

USART2 was configured in asynchronous mode with 115200 baud rate.

The STM32f407G-DISC1 send a test massage over UART through an FT232 USB-UART converter.
The message was successfully received on Ubuntu using `picocom`.

### GPIO Mapping

| Function | Pin | Description |
|---|---|---|
| USER_BUTTON | PA0 | Onboard user button |
| LD4_green | PD12 | Green onboard LED |
| LD3_orange | PD13 | Orange onboard LED |
| LD5_red | PD14 | Red onboard LED |
| LD6_red | PD15 | Blue onboard LED |

### UART Configuration

| Signal | STM32 Pin | FT232 Pin |
|---|---|---|
| USART2_TX | PA2 | RXD |
| USART2_RX | PA3 | TXD |
| GND | GND | GND |

### Serial Settings

| Parameter | Value |
|---|---|
| UART peripheral | USART2 |
| Baud rate | 115200 |
| Word length | 8 bits |
| Pariyt | None |
| Stop bits | 1 |

### Test Outputs

```text
Hello form STM32
Hello form STM32
Hello form STM32
```

### Terminal Command

```bash
picocom -b 115200 /dev/ttyUSB0
```

## Checkpoint 5 - CSV UART Data Stream

Status: Completed

The ST32F407G-DISC1 now sends structured CSV-formated data over UART.

USART2 is used to transmit system uptime, user button state, and system status to the comuter through the FT232 USB-UART converter.

### GPIO Mapping

| Function | Pin | Description |
|---|---|---|
| USER_BUTTON | PA0 | Onboard user button |
| LD4_green | PD12 | Green onboard LED |
| LD3_orange | PD13 | Orange onboard LED |
| LD5_red | PD14 | Red onboard LED |
| LD6_red | PD15 | Blue onboard LED |

### UART Configuration

| Signal | STM32 Pin | FT232 Pin |
|---|---|---|
| USART2_TX | PA2 | RXD |
| USART2_RX | PA3 | TXD |
| GND | GND | GND |

### Serial Settings

| Parameter | Value |
|---|---|
| UART peripheral | USART2 |
| Baud rate | 115200 |
| Word length | 8 bits |
| Pariyt | None |
| Stop bits | 1 |

### Output Format

```text
timestamp_ms,button_state,status
```

### Data Fields

| Field | Descriptions |
|---|---|
| timestamp_ms | System uptime in miliseconds |
| button_state | User buton state. 0 means released, 1 means prassed |
| status | System status. Currently always OK |

### Eample Outpus

```bash
64131,0,OK
65133,1,OK
66135,1,OK
67137,0,OK
68139,0,OK
69141,0,OK
```

### Test Result

The UART output was monitored on Ubuntu using `picocom`.

### Terminal Command

```bash
picocom -b 115200 /dev/ttyUSB0
```

## Checkpoit 6 - Python Serial Logger

Status: Completed

A python serial logger was implemented using `pyserial`.

The script reads CSV-formatted UART data from the STM32 through the FT232 USB-UART converter and saves the received data into a CSV file.

### Python Script

```text
python_tools/src/serial_logger.py
```
### Output Files

```text
python_tools/data/stm32_log.csv
```

### Example Saved Data

timestamp_ms,button_state,status
5013,0,OK
6015,0,OK
7017,0,OK

### Test Result

The Python logger successfully opened `/dev/ttyUSB0`, read the UART data stream, validated the CSV rows, and saved the data to `stm32_log.csv`.
