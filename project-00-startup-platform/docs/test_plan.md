# Test Plan - Project 00 Startup Platform

## Purpose

This document defines the tests used to verify the basic functionality of the STM32 startup platform.

## Test Cases

| Test ID | Test Name | Description | Expected Result |
|---|---|---|---|
| TEST-001 | Green LED Blink Test | Verify that the green onboard LED can be toggled by firmware. | The green LED blinks every 500 ms. |
| TEST-002 | Four LED Sequence Test | Verify that all four onboard LEDs can be controlled. | Green, orange, red, and blue LEDs blink in sequence. |
| TEST-003 | User Button Input Test | Verify that the onboard user button can be read as a GPIO input. | Pressing the button turns the green LED on, releasing it turns the LED off. |
| TEST-004 | UART Text Transmit Test | Verify that STM32 can send a text message over UART. | The computer receives `Hello from STM32` through FT232. |
| TEST-005 | CSV UART Data Stream Test | Verify that STM32 can send structured CSV-formatted data over UART. | The computer receives timestamp, button state, and status fields. |
| TEST-006 | Python Serial Logger Test | Verify that Python can read UART data and save it to a CSV file. | The received STM32 data is saved into a CSV file. |

## Notes

These tests were selected to verify the minimum working platform before moving to sensor-based data logging.

These tests check the basic building blocks of the project: GPIO output, GPIO input, UART communication, and Python logging.

