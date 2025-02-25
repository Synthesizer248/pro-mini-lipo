# Usage Instructions

## Home Screen
- Displays: Voltage, Current, Charged mAh, Internal Resistance, SoH.
- Rotate encoder to enter Settings.

## Charging
- From Home, press encoder to start charging (0.5C rate, 4.2V target).
- Buzzer beeps twice on start, three times (ascending) when full.

## Resistance Measurement
- In Settings, select "Calibrate" (not really, meant for mode 2), but manually set `mode = 2` in code for now (TODO: fix menu).
- Buzzer beeps once when done.

## Settings
- Adjust: Capacity (mAh), Nominal Voltage (V), R_new (Ω), R_max (Ω).
- Press to save preset (buzzer confirms).

## Calibration
- In Settings, select "Calibrate".
- Step 1: Set known voltage (e.g., 4.00V), press encoder.
- Step 2: Set known current (e.g., 500mA), press encoder.
- Buzzer beeps per step, twice when done.

## Safety
- Overvoltage (>4.3V) or undervoltage (<3.0V) triggers 5 rapid beeps and shuts off.
