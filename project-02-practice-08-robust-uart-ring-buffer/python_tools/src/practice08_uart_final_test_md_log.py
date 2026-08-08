#!/usr/bin/env python3
"""
Project 02 / Practice 08 - UART protocol final test

Dependency:
    pip install pyserial

Example:
    python3 practice08_uart_final_test.py --port /dev/ttyUSB0 --baud 115200

The overflow stress test is intentionally last because this firmware reports
RX overflow but does not implement advanced stream resynchronization afterward.
"""

import argparse
import time
import sys
from datetime import datetime
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

import serial

START = 0xAA

CMD_PING = 0x01
CMD_SET_LED = 0x02
CMD_GET_LED = 0x03
CMD_GET_ADC = 0x04

ERR_OK = 0x00
ERR_RX_TIMEOUT = 0xA0
ERR_RX_PAYLOAD_LENGTH_OVERFLOW = 0xA1
ERR_RX_START_BYTE_NOT_FOUND = 0xA2
ERR_RX_INVALID_COMMAND = 0xA3
ERR_RX_CHECKSUM = 0xA4
ERR_RX_INCONSISTENCY = 0xA5
ERR_RX_BUFFER_FULL = 0xA6
ERR_COMMAND_INVALID_PAYLOAD_LENGTH = 0xB0
ERR_COMMAND_INVALID_PAYLOAD = 0xB1
ERR_EXEC_VERIFICATION_FAILED = 0xC0
ERR_EXEC_READ_ADC_FAILED = 0xC1

ERROR_NAMES = {
    ERR_OK: "OK",
    ERR_RX_TIMEOUT: "RX_TIMEOUT",
    ERR_RX_PAYLOAD_LENGTH_OVERFLOW: "RX_PAYLOAD_LENGTH_OVERFLOW",
    ERR_RX_START_BYTE_NOT_FOUND: "RX_START_BYTE_NOT_FOUND",
    ERR_RX_INVALID_COMMAND: "RX_INVALID_COMMAND",
    ERR_RX_CHECKSUM: "RX_CHECKSUM",
    ERR_RX_INCONSISTENCY: "RX_INCONSISTENCY",
    ERR_RX_BUFFER_FULL: "RX_BUFFER_FULL",
    ERR_COMMAND_INVALID_PAYLOAD_LENGTH: "COMMAND_INVALID_PAYLOAD_LENGTH",
    ERR_COMMAND_INVALID_PAYLOAD: "COMMAND_INVALID_PAYLOAD",
    ERR_EXEC_VERIFICATION_FAILED: "EXEC_VERIFICATION_FAILED",
    ERR_EXEC_READ_ADC_FAILED: "EXEC_READ_ADC_FAILED",
}


def xor_checksum(data: bytes) -> int:
    result = 0
    for b in data:
        result ^= b
    return result


def build_request(command: int, payload: bytes = b"") -> bytes:
    body = bytes([command, len(payload)]) + payload
    return bytes([START]) + body + bytes([xor_checksum(body)])


@dataclass
class Response:
    raw: bytes
    command: int
    error: int
    length: int
    data: bytes
    checksum: int
    checksum_ok: bool

    def text(self) -> str:
        raw_hex = self.raw.hex(" ").upper()
        data_hex = self.data.hex(" ").upper() if self.data else "-"
        error_name = ERROR_NAMES.get(self.error, f"UNKNOWN_0x{self.error:02X}")
        return (
            f"RAW=[{raw_hex}] CMD=0x{self.command:02X} "
            f"ERR={error_name}(0x{self.error:02X}) LEN={self.length} "
            f"DATA=[{data_hex}] CHECKSUM={'OK' if self.checksum_ok else 'BAD'}"
        )


def read_exact(ser: serial.Serial, count: int, deadline: float) -> bytes:
    out = bytearray()
    while len(out) < count:
        if time.monotonic() >= deadline:
            raise TimeoutError(f"Need {count} bytes, received {len(out)}")
        chunk = ser.read(count - len(out))
        if chunk:
            out.extend(chunk)
    return bytes(out)


def read_response(ser: serial.Serial, timeout: float = 1.5) -> Response:
    deadline = time.monotonic() + timeout

    # Search START only while outside a response frame.
    while True:
        if time.monotonic() >= deadline:
            raise TimeoutError("No response START byte")
        b = ser.read(1)
        if b and b[0] == START:
            break

    header = read_exact(ser, 3, deadline)
    command, error, length = header

    if length > 32:
        raise ValueError(f"Unreasonable response length: {length}")

    data = read_exact(ser, length, deadline)
    checksum_byte = read_exact(ser, 1, deadline)
    checksum = checksum_byte[0]

    body = header + data
    expected_checksum = xor_checksum(body)
    raw = bytes([START]) + body + checksum_byte

    return Response(
        raw=raw,
        command=command,
        error=error,
        length=length,
        data=data,
        checksum=checksum,
        checksum_ok=(checksum == expected_checksum),
    )


class Runner:
    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self.passed = 0
        self.failed = 0
        self.warned = 0

    def pass_(self, name: str, detail: str = ""):
        self.passed += 1
        print(f"[PASS] {name}" + (f" -> {detail}" if detail else ""))

    def fail(self, name: str, detail: str = ""):
        self.failed += 1
        print(f"[FAIL] {name}" + (f" -> {detail}" if detail else ""))

    def warn(self, name: str, detail: str = ""):
        self.warned += 1
        print(f"[WARN] {name}" + (f" -> {detail}" if detail else ""))

    def flush_input(self):
        time.sleep(0.05)
        self.ser.reset_input_buffer()

    def transaction(
        self,
        name: str,
        request: bytes,
        expected_command: Optional[int],
        expected_error: int,
        expected_data: Optional[bytes] = None,
        timeout: float = 1.5,
    ) -> Optional[Response]:
        try:
            self.ser.write(request)
            self.ser.flush()
            response = read_response(self.ser, timeout=timeout)
        except Exception as exc:
            self.fail(name, str(exc))
            return None

        problems = []

        if not response.checksum_ok:
            problems.append("bad response checksum")
        if expected_command is not None and response.command != expected_command:
            problems.append(
                f"command expected 0x{expected_command:02X}, got 0x{response.command:02X}"
            )
        if response.error != expected_error:
            problems.append(
                f"error expected 0x{expected_error:02X}, got 0x{response.error:02X}"
            )
        if expected_data is not None and response.data != expected_data:
            problems.append(
                f"data expected [{expected_data.hex(' ').upper()}], "
                f"got [{response.data.hex(' ').upper()}]"
            )

        if problems:
            self.fail(name, "; ".join(problems) + " | " + response.text())
        else:
            self.pass_(name, response.text())

        return response


def test_basic_commands(r: Runner):
    print("\n=== BASIC COMMANDS ===")

    r.transaction("PING", build_request(CMD_PING), CMD_PING, ERR_OK, b"\x01")

    r.transaction(
        "SET_LED ON",
        build_request(CMD_SET_LED, b"\x01"),
        CMD_SET_LED,
        ERR_OK,
        b"\x01",
    )

    r.transaction(
        "GET_LED after ON",
        build_request(CMD_GET_LED),
        CMD_GET_LED,
        ERR_OK,
        b"\x01",
    )

    r.transaction(
        "SET_LED OFF",
        build_request(CMD_SET_LED, b"\x00"),
        CMD_SET_LED,
        ERR_OK,
        b"\x00",
    )

    r.transaction(
        "GET_LED after OFF",
        build_request(CMD_GET_LED),
        CMD_GET_LED,
        ERR_OK,
        b"\x00",
    )

    response = r.transaction(
        "GET_ADC",
        build_request(CMD_GET_ADC),
        CMD_GET_ADC,
        ERR_OK,
        expected_data=None,
    )

    if response is not None and response.error == ERR_OK:
        if response.length == 2 and len(response.data) == 2:
            adc_raw = (response.data[0] << 8) | response.data[1]
            r.pass_("GET_ADC format", f"2-byte ADC value = {adc_raw}")
        else:
            r.fail(
                "GET_ADC format",
                f"expected 2 data bytes, got LEN={response.length}",
            )


def test_protocol_errors(r: Runner):
    print("\n=== PROTOCOL / COMMAND ERRORS ===")

    r.transaction(
        "INVALID COMMAND",
        build_request(0x55),
        0x55,
        ERR_RX_INVALID_COMMAND,
        b"",
    )

    # PING requires zero payload bytes.
    r.transaction(
        "INVALID COMMAND PAYLOAD LENGTH",
        build_request(CMD_PING, b"\x00"),
        CMD_PING,
        ERR_COMMAND_INVALID_PAYLOAD_LENGTH,
        b"",
    )

    # SET_LED accepts only 0 and 1.
    r.transaction(
        "INVALID SET_LED PAYLOAD",
        build_request(CMD_SET_LED, b"\x02"),
        CMD_SET_LED,
        ERR_COMMAND_INVALID_PAYLOAD,
        b"",
    )

    bad_checksum = bytearray(build_request(CMD_PING))
    bad_checksum[-1] ^= 0xFF
    r.transaction(
        "WRONG CHECKSUM",
        bytes(bad_checksum),
        CMD_PING,
        ERR_RX_CHECKSUM,
        b"",
    )

    # Firmware rejects request LENGTH > 3 immediately after the LENGTH byte.
    r.transaction(
        "RX PAYLOAD LENGTH OVERFLOW",
        bytes([START, CMD_PING, 0x04]),
        0x00,
        ERR_RX_PAYLOAD_LENGTH_OVERFLOW,
        b"",
    )


def test_garbage_before_start(r: Runner):
    try:
        r.ser.write(b"\x00\x11\x22\x33\x44" + build_request(CMD_PING))
        r.ser.flush()
        response = read_response(r.ser, timeout=1.5)

        if (
            response.checksum_ok
            and response.command == CMD_PING
            and response.error == ERR_OK
            and response.data == b"\x01"
        ):
            r.pass_("GARBAGE BEFORE START BYTE", response.text())
        else:
            r.fail("GARBAGE BEFORE START BYTE", response.text())
    except Exception as exc:
        r.fail("GARBAGE BEFORE START BYTE", str(exc))


def test_timeout(r: Runner, response_timeout: float):
    print("\n=== FRAMING / TIMEOUT ===")
    test_garbage_before_start(r)

    try:
        # START + COMMAND only. LENGTH and CHECKSUM never arrive.
        r.ser.write(bytes([START, CMD_PING]))
        r.ser.flush()

        start_time = time.monotonic()
        response = read_response(r.ser, timeout=response_timeout)
        elapsed = time.monotonic() - start_time

        problems = []
        if not response.checksum_ok:
            problems.append("bad response checksum")
        if response.command != 0x00:
            problems.append(f"expected command 0x00, got 0x{response.command:02X}")
        if response.error != ERR_RX_TIMEOUT:
            problems.append(f"expected A0, got 0x{response.error:02X}")

        if problems:
            r.fail("INTER-BYTE TIMEOUT", "; ".join(problems) + " | " + response.text())
        else:
            r.pass_("INTER-BYTE TIMEOUT", f"{response.text()} after {elapsed:.2f}s")
    except Exception as exc:
        r.fail("INTER-BYTE TIMEOUT", str(exc))


def test_back_to_back(r: Runner):
    print("\n=== BACK-TO-BACK PACKETS ===")

    requests = (
        build_request(CMD_SET_LED, b"\x01")
        + build_request(CMD_GET_LED)
        + build_request(CMD_PING)
        + build_request(CMD_GET_ADC)
        + build_request(CMD_SET_LED, b"\x00")
        + build_request(CMD_GET_LED)
    )

    expected = [
        (CMD_SET_LED, ERR_OK, b"\x01"),
        (CMD_GET_LED, ERR_OK, b"\x01"),
        (CMD_PING, ERR_OK, b"\x01"),
        (CMD_GET_ADC, ERR_OK, None),
        (CMD_SET_LED, ERR_OK, b"\x00"),
        (CMD_GET_LED, ERR_OK, b"\x00"),
    ]

    r.ser.write(requests)
    r.ser.flush()

    problems = []

    for index, (cmd, err, data) in enumerate(expected, 1):
        try:
            response = read_response(r.ser, timeout=2.0)
        except Exception as exc:
            problems.append(f"response {index}: {exc}")
            break

        if not response.checksum_ok or response.command != cmd or response.error != err:
            problems.append(f"response {index}: {response.text()}")
            continue

        if data is not None and response.data != data:
            problems.append(f"response {index}: {response.text()}")
            continue

        if cmd == CMD_GET_ADC and response.length != 2:
            problems.append(f"response {index}: GET_ADC length != 2")

    if problems:
        r.fail("MIXED BACK-TO-BACK", " | ".join(problems))
    else:
        r.pass_("MIXED BACK-TO-BACK", f"{len(expected)} requests -> {len(expected)} valid responses")


def test_ring_wrap(r: Runner, batches: int = 4, pings_per_batch: int = 10):
    print("\n=== RING BUFFER WRAP ===")

    total_correct = 0
    ping = build_request(CMD_PING)

    for batch in range(1, batches + 1):
        r.ser.write(ping * pings_per_batch)
        r.ser.flush()

        batch_correct = 0
        for _ in range(pings_per_batch):
            try:
                response = read_response(r.ser, timeout=2.0)
            except Exception:
                break

            if (
                response.checksum_ok
                and response.command == CMD_PING
                and response.error == ERR_OK
                and response.data == b"\x01"
            ):
                batch_correct += 1

        total_correct += batch_correct

        if batch_correct != pings_per_batch:
            r.fail(
                "RING WRAP",
                f"batch {batch}: expected {pings_per_batch}, got {batch_correct}",
            )
            return

    request_bytes = batches * pings_per_batch * len(ping)
    r.pass_(
        "RING WRAP",
        f"{total_correct} PING responses correct; {request_bytes} request bytes processed",
    )


def test_overflow_notification(r: Runner, ping_count: int, read_window: float):
    print("\n=== OVERFLOW STRESS (LAST TEST) ===")
    print(
        f"Sending {ping_count} PING packets in one burst. "
        "This is intentionally the last test."
    )

    burst = build_request(CMD_PING) * ping_count
    r.ser.write(burst)
    r.ser.flush()

    deadline = time.monotonic() + read_window
    total_responses = 0
    valid_pings = 0
    overflow_notifications = 0
    bad_response_checksums = 0
    other_errors = {}

    while time.monotonic() < deadline:
        remaining = max(0.05, deadline - time.monotonic())
        try:
            response = read_response(r.ser, timeout=min(0.25, remaining))
        except TimeoutError:
            continue
        except Exception:
            continue

        total_responses += 1

        if not response.checksum_ok:
            bad_response_checksums += 1

        if response.error == ERR_RX_BUFFER_FULL:
            overflow_notifications += 1
        elif (
            response.command == CMD_PING
            and response.error == ERR_OK
            and response.data == b"\x01"
        ):
            valid_pings += 1
        elif response.error != ERR_OK:
            other_errors[response.error] = other_errors.get(response.error, 0) + 1

    detail = (
        f"responses={total_responses}, valid_ping={valid_pings}, "
        f"A6_overflow_notifications={overflow_notifications}, "
        f"bad_response_checksums={bad_response_checksums}"
    )

    if other_errors:
        detail += ", other_errors=" + ", ".join(
            f"0x{k:02X}:{v}" for k, v in sorted(other_errors.items())
        )

    if overflow_notifications > 0:
        r.pass_("BUFFER OVERFLOW NOTIFICATION", detail)
    else:
        r.warn(
            "BUFFER OVERFLOW NOTIFICATION",
            detail + " | No overflow observed; increase --overflow-pings if needed.",
        )



class Tee:
    """Write stdout/stderr to both terminal and a log file."""
    def __init__(self, *streams):
        self.streams = streams

    def write(self, data):
        for stream in self.streams:
            stream.write(data)
            stream.flush()
        return len(data)

    def flush(self):
        for stream in self.streams:
            stream.flush()


def prepare_log_file():
    """
    Expected layout:
        python_tools/
        ├── data/
        └── src/
            └── practice08_uart_final_test.py

    Test output:
        python_tools/data/final_test_YYYYMMDD_HHMMSS.md
        python_tools/data/final_test_YYYYMMDD_HHMMSS.log
    """
    script_dir = Path(__file__).resolve().parent
    python_tools_dir = script_dir.parent
    data_dir = python_tools_dir / "data"
    data_dir.mkdir(parents=True, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    md_path = data_dir / f"final_test_{timestamp}.md"
    log_path = data_dir / f"final_test_{timestamp}.log"
    return md_path, log_path


def main():
    parser = argparse.ArgumentParser(
        description="Project 02 Practice 08 UART protocol final test"
    )
    parser.add_argument("--port", required=True, help="e.g. /dev/ttyUSB0 or COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--timeout-response",
        type=float,
        default=7.0,
        help="Read window for the firmware's 5-second timeout response",
    )
    parser.add_argument(
        "--overflow-pings",
        type=int,
        default=200,
        help="PING count for the final overflow stress",
    )
    parser.add_argument(
        "--overflow-window",
        type=float,
        default=4.0,
        help="Seconds to collect responses during overflow stress",
    )
    parser.add_argument("--skip-overflow", action="store_true")
    parser.add_argument("--skip-timeout", action="store_true")
    args = parser.parse_args()

    md_path, log_path = prepare_log_file()
    md_file = md_path.open("w", encoding="utf-8", buffering=1)
    log_file = log_path.open("w", encoding="utf-8", buffering=1)

    original_stdout = sys.stdout
    original_stderr = sys.stderr

    # The same test output is visible in the terminal and stored in both files.
    sys.stdout = Tee(original_stdout, md_file, log_file)
    sys.stderr = Tee(original_stderr, md_file, log_file)

    print(f"Markdown report: {md_path}")
    print(f"Log file: {log_path}")
    print()
    print("# Project 02 / Practice 08 - UART Final Test")
    print()
    print("```text")
    print("Project 02 / Practice 08 - UART Final Test")
    print(f"Port: {args.port}")
    print(f"Baud: {args.baud}")

    with serial.Serial(
        port=args.port,
        baudrate=args.baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.05,
        write_timeout=2.0,
    ) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        runner = Runner(ser)

        test_basic_commands(runner)
        runner.flush_input()

        test_protocol_errors(runner)
        runner.flush_input()

        if args.skip_timeout:
            print("\n=== FRAMING ===")
            test_garbage_before_start(runner)
        else:
            test_timeout(runner, args.timeout_response)

        runner.flush_input()
        test_back_to_back(runner)

        runner.flush_input()
        test_ring_wrap(runner)

        if not args.skip_overflow:
            runner.flush_input()
            test_overflow_notification(
                runner,
                ping_count=args.overflow_pings,
                read_window=args.overflow_window,
            )

        print("\n=== FINAL SUMMARY ===")
        print(f"PASS : {runner.passed}")
        print(f"FAIL : {runner.failed}")
        print(f"WARN : {runner.warned}")

        print("\nNot directly force-tested:")
        print("  A2 RX_START_BYTE_NOT_FOUND  -> normally unreachable because ProcessBuffer searches 0xAA")
        print("  A5 RX_INCONSISTENCY         -> internal consistency guard")
        print("  C0 EXEC_VERIFICATION_FAILED -> requires GPIO/hardware failure")
        print("  C1 EXEC_READ_ADC_FAILED     -> requires ADC/hardware failure")

        if runner.failed == 0:
            print("\nRESULT: CORE TEST SUITE PASSED")
        else:
            print("\nRESULT: TEST SUITE FAILED")
            raise SystemExit(1)


if __name__ == "__main__":
    main()
