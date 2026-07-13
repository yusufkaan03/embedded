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
