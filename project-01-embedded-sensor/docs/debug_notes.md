# Debug Notes

## ADC value did not change as expected

During the first ADC test, the ADC value stayed around 1500 and did not follow the potentiometer position correctly. Also, the button field changed when the potentiometer was rotated.

### Cause

The potentiometer middle pin was accidentally connected to PA0 instead of PA1. PA0 is used as the USER Button input, so the analog voltage from the potentiometer affected the digital button reading.

### Fix

The potentiometer middle pin was moved to PA1, which is configured as ADC1_IN1.

### Result

After fixing the wiring, adc_raw changed from 0 to 4095 depending on the potentiometer position, and the button value only changed when the USER Button was pressed.

## UART Field Order Bug After Adding ADC Status

### Problem

After adding `adc_mv` and ADC status logic, the UART output fields became incorrect.

Expected output format:

timestamp_ms,adc_raw,adc_mv,button,status

Observed output had incorrect values in the `adc_mv` and `button` fields. The `adc_mv` field appeared as `0` or `1`, and the `button` field showed unexpected values such as `225` or `227`.

### Cause

The order of arguments in the `send_sensor_log()` function call did not match the function definition.

The function expected this order:

send_sensor_log(timestamp, adc_raw, adc_mv, button, situation);

But it was accidentally called like this:

send_sensor_log(timestamp, adc_raw, button, adc_mv, situation);

Because `button` is a `uint8_t`, the `larger adc_mv` value was truncated when passed into the button parameter. This caused incorrect UART output.

### Fix

The function call was corrected to match the function definition:

send_sensor_log(now, adc_raw, adc_mv, button, current_situation);

### Result

After fixing the parameter order, the UART output returned to the correct format:

timestamp_ms,adc_raw,adc_mv,button,status

The `adc_mv` value was calculated correctly, the `button` field returned to `0` or `1`, and the status field correctly showed `LOW`, `OK`, or `HIGH`.

### Lesson Learned

When adding new parameters to a function, the function definition and function call order must be checked carefully. In C, mismatched parameter order can compile successfully but still produce incorrect runtime behavior.

## CubeMX Code Generation Removed Main Loop Code

After configuring additional GPIO pins for status LEDs and regenerating code with CubeMX, the application code inside the `while (1)` loop was removed.

### Cause

The code was placed between `/* USER CODE END WHILE */` and `/* USER CODE BEGIN 3 */`, which is not protected by CubeMX.

### Fix

The loop application code was moved inside the protected `/* USER CODE BEGIN 3 */` section.

### Lesson Learned

User code must be written only inside CubeMX-protected `USER CODE BEGIN` / `USER CODE END` blocks. Otherwise, code generation may overwrite or remove it.

## Sensor Model Note

The connected sensor module was initially expected to be a BME280. During chip ID testing, the sensor returned chip ID `0x58`.

Chip ID meanings:

```text
0x60 -> BME280
0x58 -> BMP280
```

This confirms that the connected module is a BMP280. Therefore, this project will read temperature and pressure only. Humidity measurement is not supported by the current sensor.

## Duplicate I2C Scan Output

During testing, the UART terminal seemed to show the I2C scan output twice.

### Cause

The first scan output was leftover/partial UART output from the previous run. After adding a boot marker (`--- BOOT ---`), it became clear that the firmware only performed one scan after each boot.

### Result

No code issue was found. The I2C scan runs once during startup.

## BMP280 Raw Measurement Registers Stuck

### Problem

The BMP280 sensor was detected correctly over I2C, but raw temperature and pressure values stayed fixed.

Observed output:

```text
raw bytes: FF FF 00 FF FF 00
BMP280 raw: temp=1048560, press=1048560
```

### Tests Performed

* I2C scan found the sensor at `0x76`
* Chip ID register returned `0x58`
* Sensor was detected as BMP280
* Calibration registers were read successfully
* CSB was connected to `3.3V`
* SDO was connected to `GND`
* Forced measurement was triggered through `ctrl_meas`
* Software reset was tested
* Raw pressure and temperature registers were read from `0xF7–0xFC`

Then SDO was connected to `3.3V` and the I2C address was changed to `0x77`.

The sensor was again detected correctly:

```text
Device found at 0x77
Chip ID: 0x58
Sensor detected: BMP280
```

However, the raw measurement registers still returned:

```text
raw bytes: FF FF 00 FF FF 00
```

### Conclusion

The STM32 I2C communication, register read/write operations, chip ID read, calibration read, software reset, and forced measurement command are working.

The raw measurement output registers remain stuck, which suggests a possible sensor module, breakout board, or clone hardware issue.

For this project, the BMP280 measurement issue is documented as a hardware/module limitation. The project can continue with the working ADC logger and documented BMP280 bring-up/debug process.
