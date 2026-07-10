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
