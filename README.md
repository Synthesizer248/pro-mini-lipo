# LiPo Charger Arduino

A DIY LiPo/Li-ion battery charger with internal resistance measurement and State of Health (SoH) estimation, built using an Arduino Pro Mini.

## Features
- Adjustable charging voltage/current via LM2596 (PWM-controlled).
- Measures battery voltage, current, and internal resistance using INA219.
- Displays data on a 0.96" I2C OLED.
- Menu navigation via rotary encoder with buzzer feedback.
- Stores 4 battery presets in EEPROM.
- Calibration mode for INA219 accuracy.
- Safety features: reverse polarity protection, watchdog timer, over/under-voltage alerts.

## Hardware Requirements
- Arduino Pro Mini (5V, 16MHz)
- INA219 Current/Voltage Sensor
- LM2596 DC-DC Buck Converter (modified for PWM control)
- 0.96" I2C OLED Display
- Rotary Encoder with Push Button
- 12V 3A Power Supply
- Schottky Diode, Buzzer, MOSFET (IRF540), 0.1Ω Shunt, 10Ω Load Resistor
- Capacitors, resistors, etc. (see `docs/hardware.md`)

## Setup
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Synthesizer248/pro-mini-lipo.git
2.Install Libraries:
-Adafruit_INA219
-U8g2
3.Encoder Install via Arduino IDE Library Manager.
4.Upload Code: Open src/LiPoCharger.ino in Arduino IDE and upload to your Pro Mini.
5.Assemble Hardware: Follow docs/hardware.md for wiring details.
6.Test: Power on, use the encoder to navigate, and charge a battery.
