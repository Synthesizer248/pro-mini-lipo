#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "battery_preset.h"

// Display states
enum DisplayState {
    STATUS_SCREEN,
    VOLTAGE_SCREEN,
    CURRENT_SCREEN,
    BATTERY_SCREEN,
    ERROR_SCREEN
};

class Display {
public:
    Display();
    void begin();
    void showChargeStatus(float voltage, float current, float setVoltage, float setCurrent);
    void showVoltageSet(float voltage);
    void showCurrentSet(float current);
    void showDetailedStatus(float voltage, float current, float setVoltage, float setCurrent, 
                          uint8_t state, float mAh, unsigned long elapsedTime);
    void showBatteryPresets();
    void showPresetMenu(const BatteryPreset* presets, uint8_t count);
    void showPresetEdit(const BatteryPreset& preset);
    void showError(const char* message);
    void clear();
    void update();
    
private:
    Adafruit_PCD8544 lcd;
    DisplayState currentState;
    void drawBatteryIcon(int x, int y, int percentage);
    void formatTime(unsigned long ms, char* buffer);
    void drawProgressBar(int x, int y, int width, int height, int percentage);
    void drawHeader(const char* title);
};