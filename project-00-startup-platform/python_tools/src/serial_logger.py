import csv
from pathlib import Path

import serial


SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 115200

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
CSV_FILE = DATA_DIR / "stm32_log.csv"

EXPECTED_HEADER = ["timestamp_ms", "button_state", "status"]


def is_valid_data_row(parts):
    if len(parts) != 3:
        return False

    timestamp_ms, button_state, status = parts

    if not timestamp_ms.isdigit():
        return False

    if button_state not in ["0", "1"]:
        return False

    if status != "OK":
        return False

    return True


def main():
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    print(f"Opening serial port: {SERIAL_PORT} at {BAUD_RATE} baud")
    print(f"Saving data to: {CSV_FILE}")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    except serial.SerialException as error:
        print(f"ERROR: Could not open serial port: {error}")
        return

    with ser, open(CSV_FILE, mode="w", newline="") as file:
        writer = csv.writer(file)

        writer.writerow(EXPECTED_HEADER)
        file.flush()

        print("CSV header written by Python logger.")

        while True:
            try:
                line = ser.readline().decode("utf-8", errors="replace").strip()

                if not line:
                    print("WARNING: No data received")
                    continue

                print(line)

                parts = line.split(",")

                if parts == EXPECTED_HEADER:
                    print("Header received from STM32. Skipping duplicate header.")
                    continue

                if not is_valid_data_row(parts):
                    print(f"WARNING: Invalid data format: {line}")
                    continue

                writer.writerow(parts)
                file.flush()

            except KeyboardInterrupt:
                print("\nLogger stopped by user.")
                break

            except serial.SerialException as error:
                print(f"ERROR: Serial communication error: {error}")
                break


if __name__ == "__main__":
    main()