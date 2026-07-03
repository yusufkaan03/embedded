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
