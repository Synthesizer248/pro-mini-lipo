#include "power.h"

#define TEMP_PIN A0
#define TEMP_R1 10000    // 10k pullup resistor
#define TEMP_BETA 3950   // NTC beta value
#define TEMP_NOMINAL 25  // Room temperature in Celsius

Power::Power(uint8_t pin) {
    pwmPin = pin;
    pidIntegral = 0;
    lastError = 0;
    safetyCheck = true;
}

void Power::begin() {
    pinMode(pwmPin, OUTPUT);
    pinMode(TEMP_PIN, INPUT);
    analogWrite(pwmPin, 0);
    
    if (!ina219.begin()) {
        safetyCheck = false;
        Serial.println(F("Failed to find INA219 chip"));
        return;
    }
    calibrate();
}

void Power::calibrate() {
    ina219.setCalibration_32V_2A();
    delay(100);
    float shuntVoltage = ina219.getShuntVoltage_mV();
    if (abs(shuntVoltage) < 0.01) {
        ina219.setCalibration_16V_400mA();
    }
}

void Power::reset() {
    safetyCheck = true;
    pidIntegral = 0;
    lastError = 0;
    analogWrite(pwmPin, 0);
}

bool Power::isConnected() {
    float voltage = getVoltage();
    return (voltage > 0.5 && voltage < MAX_CELL_VOLTAGE * 4);
}

float Power::getTemperature() {
    int rawTemp = analogRead(TEMP_PIN);
    float resistance = TEMP_R1 * (1023.0 / rawTemp - 1.0);
    float steinhart = log(resistance / TEMP_R1);
    steinhart /= TEMP_BETA;
    steinhart += 1.0 / (TEMP_NOMINAL + 273.15);
    return (1.0 / steinhart) - 273.15;
}

void Power::regulate(float targetVoltage, float targetCurrent) {
    if (!safetyCheck || !isConnected()) return;
    
    currentVoltage = getVoltage();
    currentCurrent = getCurrent();
    float temp = getTemperature();
    
    if (temp > MAX_TEMPERATURE) {
        stopCharging();
        return;
    }
    
    float error = targetVoltage - currentVoltage;
    pidIntegral = constrain(pidIntegral + error, -255, 255);
    float derivative = error - lastError;
    
    int pwmValue = (PID_KP * error) + (PID_KI * pidIntegral) + (PID_KD * derivative);
    pwmValue = constrain(pwmValue, 0, 255);
    
    if (currentCurrent > targetCurrent) {
        pwmValue = max(0, pwmValue - 10);
        pidIntegral = 0;
    }
    
    lastError = error;
    analogWrite(pwmPin, pwmValue);
}

bool Power::checkSafety() {
    currentVoltage = getVoltage();
    currentCurrent = getCurrent();
    
    if (currentVoltage > MAX_CELL_VOLTAGE || 
        currentVoltage < MIN_CELL_VOLTAGE || 
        currentCurrent > MAX_CHARGE_CURRENT) {
        stopCharging();
        return false;
    }
    return true;
}

void Power::stopCharging() {
    analogWrite(pwmPin, 0);
    safetyCheck = false;
}
float Power::getVoltage() {
    return ina219.getBusVoltage_V();
}

float Power::getCurrent() {
    return ina219.getCurrent_mA() / 1000.0;
}
float Power::getPower() {
    return ina219.getPower_mW() / 1000.0;
}