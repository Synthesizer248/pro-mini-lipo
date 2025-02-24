#pragma once
#include <Arduino.h>

class Encoder {
public:
    Encoder(uint8_t clk, uint8_t dt, uint8_t sw);
    void begin();
    void update();
    int getDelta();
    bool isShortPress();
    bool isLongPress();
private:
    uint8_t pinCLK;
    uint8_t pinDT;
    uint8_t pinSW;
    int lastCLK;
    int position;
    int lastPosition;
    unsigned long lastButtonPress;
    bool buttonState;
};