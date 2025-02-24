#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "display.h"
#include "encoder.h"
#include "power.h"
#include "battery_preset.h"

// Pin definitions
#define PWM_PIN 9
#define ENCODER_CLK 2
#define ENCODER_DT 3
#define ENCODER_SW 4
#define LED_CHARGING 10
#define LED_COMPLETE 11
#define LED_ERROR 12
#define BUZZER_PIN 8

// Safety thresholds for single cell
#define CHARGE_COMPLETE_CURRENT 0.1
// Remove MAX_CELL_VOLTAGE as it's already defined in power.h
#define MIN_CELL_VOLTAGE 2.5
#define CV_PHASE_THRESHOLD 4.1

// Sound functions
void playTone(unsigned int frequency, unsigned long duration) {
    tone(BUZZER_PIN, frequency, duration);
}

void playAlert() {
    tone(BUZZER_PIN, 2000, 100);
    delay(100);
    tone(BUZZER_PIN, 1500, 100);
}

void playClick() {
    tone(BUZZER_PIN, 4000, 5);
}

void playTick() {
    tone(BUZZER_PIN, 3000, 2);
}

enum ChargingState {
    IDLE,
    CONSTANT_CURRENT,
    CONSTANT_VOLTAGE,
    COMPLETE,
    ERROR
};

enum MenuState {
    MAIN_DISPLAY,
    SELECT_PRESET,
    EDIT_PRESET,
    SET_VOLTAGE,
    SET_CURRENT
};

// Global variables
ChargingState chargingState = IDLE;
MenuState menuState = MAIN_DISPLAY;
static float setVoltage = 4.2;
static float setCurrent = 0.5;
static float mAhCharged = 0.0;
static unsigned long chargeStartTime = 0;
static unsigned long lastCurrentUpdate = 0;
static bool charging = false;
static uint8_t selectedPreset = 0;

// Objects
Display display;
Encoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);
Power power(PWM_PIN);

BatteryPreset BATTERY_PRESETS[4] = {
    {"LiPo 4.2V", 4.20, 1.0},   // Standard LiPo
    {"Li-Ion 4.1V", 4.10, 0.5}, // Standard Li-Ion
    {"LiFePO4 3.6V", 3.65, 0.5},// LiFePO4
    {"Li-HV 4.35V", 4.35, 0.5}  // High Voltage Li-Po
};

void savePreset(uint8_t index) {
    if (index >= 4) return;
    // Start EEPROM address at 0 and calculate offset for each preset
    int addr = 0 + sizeof(float) * 2 * index;
    const uint8_t EEPROM_VALID_FLAG = 0xAA;  // Define validation flag
    EEPROM.write(addr, EEPROM_VALID_FLAG);
    EEPROM.put(addr + 1, BATTERY_PRESETS[index].voltage);
    EEPROM.put(addr + 1 + sizeof(float), BATTERY_PRESETS[index].current);
}

void loadPresets() {
    const uint8_t EEPROM_VALID_FLAG = 0xAA;  // Define validation flag
    for (uint8_t i = 0; i < 4; i++) {
        int addr = 0 + sizeof(float) * 2 * i;  // Start from address 0
        if (EEPROM.read(addr) == EEPROM_VALID_FLAG) {
            EEPROM.get(addr + 1, BATTERY_PRESETS[i].voltage);
            EEPROM.get(addr + 1 + sizeof(float), BATTERY_PRESETS[i].current);
        }
    }
}

void updateChargingState() {
    if (!charging) return;
    
    float voltage = power.getVoltage();
    float current = power.getCurrent();
    
    // Single cell voltage safety check
    if (voltage > MAX_CELL_VOLTAGE || voltage < MIN_CELL_VOLTAGE) {
        chargingState = ERROR;
        charging = false;
        digitalWrite(LED_ERROR, HIGH);
        playAlert();  // Add alert sound
        return;
    }
    
    // Calculate mAh
    unsigned long now = millis();
    if (lastCurrentUpdate != 0) {
        float hours = (now - lastCurrentUpdate) / 3600000.0;
        mAhCharged += current * hours * 1000.0;
    }
    lastCurrentUpdate = now;
    
    switch(chargingState) {
        case IDLE:
            if (power.isConnected()) {
                chargingState = CONSTANT_CURRENT;
                chargeStartTime = millis();
                digitalWrite(LED_CHARGING, HIGH);
                playClick();  // Feedback for state change
            }
            break;
            
        case CONSTANT_CURRENT:
            if (voltage >= CV_PHASE_THRESHOLD) {
                chargingState = CONSTANT_VOLTAGE;
                playAlert();  // Alert for CV mode
            }
            break;
            
        case CONSTANT_VOLTAGE:
            if (current <= CHARGE_COMPLETE_CURRENT) {
                chargingState = COMPLETE;
                charging = false;
                digitalWrite(LED_CHARGING, LOW);
                digitalWrite(LED_COMPLETE, HIGH);
                playAlert();  // Alert for completion
            }
            break;
            
        case COMPLETE:
        case ERROR:
            break;
    }
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    
    pinMode(LED_CHARGING, OUTPUT);
    pinMode(LED_COMPLETE, OUTPUT);
    pinMode(LED_ERROR, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    
    display.begin();
    encoder.begin();
    power.begin();
    loadPresets();
}

void loop() {
    encoder.update();
    
    // Add encoder sound feedback
    if(encoder.getDelta() != 0) {
        playTick();
    }
    
    switch(menuState) {
        case MAIN_DISPLAY:
            display.showDetailedStatus(
                power.getVoltage(),
                power.getCurrent(),
                setVoltage,
                setCurrent,
                chargingState,
                mAhCharged,
                millis() - chargeStartTime
            );
            if(encoder.isShortPress()) menuState = SELECT_PRESET;
            if(encoder.isLongPress()) {
                charging = !charging;
                if (!charging) {
                    chargingState = IDLE;
                    mAhCharged = 0;
                    digitalWrite(LED_CHARGING, LOW);
                    digitalWrite(LED_COMPLETE, LOW);
                    power.reset();
                }
            }
            break;
            
        case SELECT_PRESET:
            display.showPresetMenu(BATTERY_PRESETS, 4);
            selectedPreset = (selectedPreset + encoder.getDelta()) % 4;
            if (selectedPreset < 0) selectedPreset += 4;
            
            if(encoder.isShortPress()) {
                setVoltage = BATTERY_PRESETS[selectedPreset].voltage;
                setCurrent = BATTERY_PRESETS[selectedPreset].current;
                menuState = MAIN_DISPLAY;
            }
            if(encoder.isLongPress()) {
                menuState = EDIT_PRESET;
            }
            break;
            
        case EDIT_PRESET:
            display.showPresetEdit(BATTERY_PRESETS[selectedPreset]);
            if(encoder.isShortPress()) {
                savePreset(selectedPreset);
                menuState = SELECT_PRESET;
            }
            if(encoder.isLongPress()) {
                menuState = SELECT_PRESET;
            }
            setVoltage += encoder.getDelta() * 0.1;
            setVoltage = constrain(setVoltage, 3.0, 4.2);
            BATTERY_PRESETS[selectedPreset].voltage = setVoltage;
            break;
            
        case SET_VOLTAGE:
            display.showVoltageSet(setVoltage);
            if(encoder.isShortPress()) menuState = SET_CURRENT;
            if(encoder.isLongPress()) menuState = MAIN_DISPLAY;
            setVoltage += encoder.getDelta() * 0.1;
            setVoltage = constrain(setVoltage, 2.5, 4.35);
            break;
            
        case SET_CURRENT:
            display.showCurrentSet(setCurrent);
            if(encoder.isShortPress()) menuState = MAIN_DISPLAY;
            if(encoder.isLongPress()) menuState = MAIN_DISPLAY;
            setCurrent += encoder.getDelta() * 0.1;
            setCurrent = constrain(setCurrent, 0.1, 2.0);
            break;
    }
    
    updateChargingState();
    
    if (charging && chargingState != ERROR) {
        power.regulate(setVoltage, setCurrent);
    }
    
    delay(100);
}