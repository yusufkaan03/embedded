# Test Results

## Test 01 - ADC Raw and Millivolt Output

### Goal

Verify that the STM32 reads the potentiometer using ADC1_IN1 and sends both raw ADC value and calculated millivolt value over UART.

### UART Output Format

```text
timestamp_ms,adc_raw,adc_mv,button,status
```

### Sample Output

```bash
34000,876,705,0,OK
35000,2813,2266,0,OK
36000,4095,3300,0,OK
37000,4095,3300,0,OK
38000,2687,2165,0,OK
39000,380,306,0,OK
40000,0,0,0,OK
41000,3,2,0,OK
42000,2,1,0,OK
43000,3,2,1,OK
44000,3,2,0,OK
```

### Result

The ADC raw value changed according to the potentiometer position.

- Minimum position:`adc_raw`was close to`0`
- Middle range:`adc_raw changed` between approximately `800` and `2800`
- Maximum position: `adc_raw` reached `4095`
- `adc_mv` changed between approximately `0 mV` and `3300 mV`
- USER Button value changed from `0` to `1` when pressed

## Test 02 - ADC Averaging

Verify that the firmware reads multiple ADC samples and sends the averaged ADC value over UART.

### UART Output Format

timestamp_ms,adc_raw,adc_mv,button,status

### Observed UART Output

```bash
12000,5,4,0,OK
13000,3,2,0,OK
14000,1643,1324,0,OK
15000,3535,2848,0,OK
16000,4094,3299,0,OK
17000,4095,3300,0,OK
18000,4094,3299,0,OK
19000,4095,3300,0,OK
20000,4094,3299,0,OK
21000,2210,1780,0,OK
22000,2206,1777,0,OK
23000,2211,1781,1,OK
24000,2485,2002,1,OK
25000,4094,3299,1,OK
26000,4095,3300,1,OK
```

### Result

The averaged ADC value changed according to the potentiometer position. When the potentiometer was kept at the maximum position, `adc_raw` stayed around `4094–4095`. In the middle range, the value stayed around `2206–2211`, which shows a stable reading.

The calculated `adc_mv` value also changed correctly between approximately `0 mV` and `3300 mV`.

### Note

During testing, a UART field order bug was found after adding the `adc_mv` parameter. The `button` and `adc_mv` arguments were passed in the wrong order. After correcting the function call, the UART output fields were verified again and the test passed.

## Test 04 - Status LED Behavior

### Goal

Verify that the onboard LEDs indicate the ADC status correctly.

### Status LED Mapping

```text
LOW   -> Blue LED
OK    -> Green LED
HIGH  -> Orange LED
ERROR -> Red LED
```

### Result

The LED behavior changed according to the ADC status:

* When `adc_mv` was below `500 mV`, the Blue LED turned on.
* When `adc_mv` was between `500 mV` and `3000 mV`, the Green LED turned on.
* When `adc_mv` was above `3000 mV`, the Orange LED turned on.
* Other LEDs were turned off before enabling the active status LED.

## Test 05 - BME280 I2C Bring-up

### Goal

Verify that STM32 can detect the BME280 sensor on the I2C bus.

### Configuration

```text
I2C Peripheral: I2C2
SCL: PB10
SDA: PB11
BME280 Address: 0x76
```

### Observed UART Output

```text
I2C scan started
Device found at 0x76
I2C scan finished
timestamp_ms,adc_raw,adc_mv,button,status
1000,4060,3271,0,HIGH
2000,4059,3270,0,HIGH
3000,4059,3270,0,HIGH
4000,4059,3270,0,HIGH
```

### Result

The STM32 successfully detected the BME280 sensor at I2C address `0x76`. The existing ADC UART logger continued to work after enabling I2C2.

## Test 06 - Sensor Chip ID Read

### Goal

Verify that STM32 can read the sensor chip ID register over I2C.

### Register

```text
Chip ID register: 0xD0
```

### Observed UART Output

I2C scan started
Device found at 0x76
I2C scan finished
BME280 chip ID: 0x58
BME280 chip ID check: ERROR

### Result

The STM32 successfully read the chip ID register over I2C. The returned chip ID was 0x58.

According to the sensor ID values:

0x60 -> BME280
0x58 -> BMP280

This indicates that the connected module is most likely a BMP280, not a BME280.

## Test 06 - BMP280 Chip ID Read

### Goal

Verify that STM32 can read the sensor chip ID over I2C and identify the connected sensor.

### Observed UART Output

```text
--- BOOT ---
I2C scan started
Device found at 0x76
I2C scan finished
Chip ID: 0x58
Sensor detected: BMP280
timestamp_ms,adc_raw,adc_mv,button,status
```

### Result

The STM32 successfully read chip ID 0x58 from register 0xD0. This confirms that the connected sensor is a BMP280. The ADC logger continued running after the sensor detection step.

## Test 07 - BMP280 Calibration Register Read

### Goal

Verify that STM32 can read the BMP280 calibration registers over I2C.

### Observed UART Output

```text
--- BOOT ---
I2C scan started
Device found at 0x76
I2C scan finished
Chip ID: 0x58
Sensor detected: BMP280
BMP280 calibration data:
dig_T1=27550
dig_T2=26576
dig_T3=-1000
dig_P1=36988
dig_P2=-10586
dig_P3=3024
dig_P4=2570
dig_P5=315
dig_P6=-7
dig_P7=15500
dig_P8=-14600
dig_P9=6000
timestamp_ms,adc_raw,adc_mv,button,status
1000,4056,3268,0,HIGH
2000,4057,3269,0,HIGH
3000,4057,3269,0,HIGH
4000,4056,3268,0,HIGH
```

### Result

The STM32 successfully read the BMP280 calibration registers starting from register `0x88`.

The calibration values were not zero and included both unsigned and signed values. This shows that the I2C register read and calibration data parsing are working correctly.

The existing ADC UART logger continued to work after the BMP280 calibration read.

