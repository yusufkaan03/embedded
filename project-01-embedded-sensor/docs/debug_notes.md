# Debug Notes

## ADC value did not change as expected

During the first ADC test, the ADC value stayed around 1500 and did not follow the potentiometer position correctly. Also, the button field changed when the potentiometer was rotated.

### Cause

The potentiometer middle pin was accidentally connected to PA0 instead of PA1. PA0 is used as the USER Button input, so the analog voltage from the potentiometer affected the digital button reading.

### Fix

The potentiometer middle pin was moved to PA1, which is configured as ADC1_IN1.

### Result

After fixing the wiring, adc_raw changed from 0 to 4095 depending on the potentiometer position, and the button value only changed when the USER Button was pressed.
