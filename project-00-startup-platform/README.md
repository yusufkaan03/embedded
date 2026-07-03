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
