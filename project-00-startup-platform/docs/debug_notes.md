# Debug Notes - Project 00 Startup Platform

## 1. STM32CubeIDE and STM32CubeMX Workflow Issue

### Problem

At first, I expected STM32CubeIDE to include the CubeMX pin configuration interface inside the IDE, because I had used that workflow before.

However, in STM32CubeIDE 2.x, STM32CubeMX is no longer integrated inside STM32CubeIDE. I could not find the usual pin configuration and `.ioc` editing interface inside CubeIDE.

### Debug Method

I checked the STM32CubeIDE and STM32CubeMX workflow and learned that STM32CubeMX must be installed and used as a standalone tool.

### Solution

I installed STM32CubeMX separately and used this workflow:

```text
STM32CubeMX -> Pin and peripheral configuration
STM32CubeIDE -> Code editing, build, debug, and programming
```

---

## 2. STM32CubeMX Launcher Issue on Ubuntu

### Problem

STM32CubeMX was installed, but it did not appear in the application launcher. Running `STM32CubeMX` directly from the terminal also did not work.

### Debug Method

I searched for the installation path using:

```bash
find /opt /usr/local ~/ -iname "*CubeMX*" 2>/dev/null | head -50
```

The executable was found at:

```text
/usr/local/STMicroelectronics/STM32Cube/STM32CubeMX/STM32CubeMX
```

### Solution

I started CubeMX manually from its installation directory:

```bash
cd /usr/local/STMicroelectronics/STM32Cube/STM32CubeMX
./STM32CubeMX
```

Later, a wrapper script was created so that CubeMX could be started from the terminal more easily.

---

## 3. CubeIDE Workspace Path Conflict

### Problem

After generating code from STM32CubeMX, STM32CubeIDE gave an error because the workspace path and project path were the same.

### Debug Method

I realized that the CubeIDE workspace should not be the same folder as the STM32 project folder.

The STM32 project folder is part of the GitHub repository, but the CubeIDE workspace is only used by the IDE for its own metadata.

### Solution

I created a separate workspace folder:

```bash
mkdir -p ~/stm32cubeide-workspace
```

Then I imported the existing STM32 project into CubeIDE from the firmware project folder.

---

## 4. UART Build Error

### Problem

The first UART build failed because of typing mistakes in the code.

Incorrect code:

```c
HAL_UART_Transmit(&huart2, (unit8_t*)msg, strlem(msg), HAL_MAX_DELAY);
```

The errors were caused by:

```text
unit8_t  -> should be uint8_t
strlem   -> should be strlen
```

### Debug Method

The compiler error message pointed to the incorrect line in `main.c`.

The build output showed that `unit8_t` was undeclared and suggested `uint8_t`.

### Solution

The code was corrected as:

```c
HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
```

Also, this include was required for `strlen()`:

```c
#include <string.h>
```

---

## 5. FT232 Serial Port Detection

### Problem

At first, `/dev/ttyUSB0` was not visible on Ubuntu.

The command:

```bash
ls /dev/ttyUSB*
```

returned:

```text
No such file or directory
```

### Debug Method

I checked whether the FT232 adapter was detected by the system using:

```bash
lsusb
```

The FT232 adapter appeared as:

```text
Future Technology Devices International, Ltd FT232 Serial (UART) IC
```

Then I checked serial devices again:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

### Solution

After reconnecting the FT232 adapter, `/dev/ttyUSB0` appeared and was used for UART communication.

The final UART monitoring command was:

```bash
picocom -b 115200 /dev/ttyUSB0
```

---

## 6. Python Logger Missed the CSV Header

### Problem

The first Python logger waited for the STM32 CSV header:

```text
timestamp_ms,button_state,status
```

If STM32 was already running before the Python script started, the header was missed. The logger then waited for the header and did not save the incoming data rows.

### Debug Method

The terminal showed valid data rows such as:

```text
8019,0,OK
9021,0,OK
10023,0,OK
```

but also printed:

```text
WARNING: Waiting for valid CSV header...
```

This showed that the UART data was arriving correctly, but the Python script logic was too strict.

### Solution

The Python logger was improved to write the expected CSV header itself and then save valid data rows.

The logger now writes this header:

```text
timestamp_ms,button_state,status
```

and accepts rows like:

```text
5013,0,OK
6015,0,OK
7017,0,OK
```

---

## What I Learned

- STM32CubeMX and STM32CubeIDE have separate roles in the current workflow.
- CubeMX is used for pin, clock, and peripheral configuration.
- CubeIDE is used for code editing, building, debugging, and programming the board.
- CubeIDE workspace and project folders should be kept separate.
- Small typing mistakes in embedded C can break the build.
- UART debugging should be checked step by step: peripheral configuration, wiring, baud rate, serial port, and terminal output.
- A Python logger should be robust against missed headers and unexpected serial data.
- Git commits help preserve each working checkpoint.
