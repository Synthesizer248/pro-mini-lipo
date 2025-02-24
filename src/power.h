#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// Safety thresholds
#define MAX_CELL_VOLTAGE 4.25
#define MIN_CELL_VOLTAGE 2.5
#define MAX_CHARGE_CURRENT 2.0
#define CV_PHASE_THRESHOLD 4.1
#define CHARGE_COMPLETE_CURRENT 0.1
#define MAX_TEMPERATURE 45.0

// PID constants
#define PID_KP 50.0
#define PID_KI 0.5
#define PID_KD 1.0

class Power {
private:
    uint8_t pwmPin;
    Adafruit_INA219 ina219;
    float currentVoltage;
    float currentCurrent;
    bool safetyCheck;
    float pidIntegral;
    float lastError;
    
public:
    Power(uint8_t pin);
    void begin();
    void regulate(float targetVoltage, float targetCurrent);
    float getVoltage();
    float getCurrent();
    float getPower();
    bool checkSafety();
    void stopCharging();
    void reset();
    bool isConnected();
    void calibrate();
    float getTemperature();
};