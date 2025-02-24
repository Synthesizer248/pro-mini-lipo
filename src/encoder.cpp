#include "encoder.h"

#define DEBOUNCE_TIME 50
#define LONG_PRESS_TIME 1000

Encoder::Encoder(uint8_t clk, uint8_t dt, uint8_t sw) {
    pinCLK = clk;
    pinDT = dt;
    pinSW = sw;
    lastCLK = 0;
    position = 0;
    lastPosition = 0;
    lastButtonPress = 0;
    buttonState = true;
}

void Encoder::begin() {
    pinMode(pinCLK, INPUT);
    pinMode(pinDT, INPUT);
    pinMode(pinSW, INPUT_PULLUP);
    lastCLK = digitalRead(pinCLK);
}

void Encoder::update() {
    // Handle rotation
    int currentCLK = digitalRead(pinCLK);
    if (currentCLK != lastCLK && currentCLK == 1) {
        if (digitalRead(pinDT) != currentCLK) {
            position--;
        } else {
            position++;
        }
    }
    lastCLK = currentCLK;

    // Handle button
    bool currentState = digitalRead(pinSW);
    if (currentState != buttonState && millis() - lastButtonPress > DEBOUNCE_TIME) {
        buttonState = currentState;
        if (buttonState == LOW) {
            lastButtonPress = millis();
        }
    }
}

int Encoder::getDelta() {
    int delta = position - lastPosition;
    lastPosition = position;
    return delta;
}

bool Encoder::isShortPress() {
    unsigned long pressDuration = millis() - lastButtonPress;
    return (!buttonState && pressDuration > DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME);
}

bool Encoder::isLongPress() {
    return (!buttonState && (millis() - lastButtonPress) >= LONG_PRESS_TIME);
}