import csv
from pathlib import Path

import serial


SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 115200
MAX_VALID_ROWS = 20

EXPECTED_HEADER = ["timestamp_ms", "mode", "button_count", "status"]
VALID_MODES = ["IDLE", "ACTIVE", "ERROR_SIM"]


DATA_DIR = Path(__file__).resolve().parent.parent / "data"
CSV_FILE = DATA_DIR / "mode_log.csv"


def is_valid_data_row(parts):
    """
    Expected format:
    timestamp_ms,mode,button_count,status

    Example:
    3005,IDLE,0,OK
    9011,ACTIVE,1,OK
    13015,ERROR_SIM,2,ERROR
    """

    if len(parts) != 4:
        return False

    timestamp_ms, mode, button_count, status = parts

    if not timestamp_ms.isdigit():
        return False

    if mode not in VALID_MODES:
        return False

    if not button_count.isdigit():
        return False

    if status not in ["OK", "ERROR"]:
        return False

    if mode == "ERROR_SIM" and status != "ERROR":
        return False

    if mode in ["IDLE", "ACTIVE"] and status != "OK":
        return False

    return True


def main():
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    print(f"Opening serial port: {SERIAL_PORT}")
    print(f"Baud rate: {BAUD_RATE}")
    print(f"Saving valid rows to: {CSV_FILE}")
    print(f"Target valid row count: {MAX_VALID_ROWS}")
    print()

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    except serial.SerialException as error:
        print(f"ERROR: Could not open serial port.")
        print(f"Details: {error}")
        return

    valid_row_count = 0

    with ser, open(CSV_FILE, mode="w", newline="") as file:
        writer = csv.writer(file)

        writer.writerow(EXPECTED_HEADER)
        file.flush()

        print("CSV header written by Python logger.")
        print("Reading STM32 UART data...")
        print()

        while valid_row_count < MAX_VALID_ROWS:
            try:
                line = ser.readline().decode("utf-8", errors="replace").strip()

                if not line:
                    print("WARNING: No data received")
                    continue

                print(line)

                parts = [item.strip() for item in line.split(",")]

                if parts == EXPECTED_HEADER:
                    print("Header received from STM32. Skipping duplicate header.")
                    continue

                if not is_valid_data_row(parts):
                    print(f"WARNING: Invalid data format skipped: {line}")
                    continue

                writer.writerow(parts)
                file.flush()

                valid_row_count += 1
                print(f"Saved valid row: {valid_row_count}/{MAX_VALID_ROWS}")

            except KeyboardInterrupt:
                print()
                print("Logger stopped by user.")
                break

            except serial.SerialException as error:
                print(f"ERROR: Serial communication error: {error}")
                break

    print()
    print(f"Logger finished. Total valid rows saved: {valid_row_count}")
    print(f"CSV file: {CSV_FILE}")


if __name__ == "__main__":
    main()