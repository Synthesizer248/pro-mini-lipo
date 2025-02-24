#include "display.h"
#include "battery_preset.h"

// Nokia 5110 LCD pins
#define LCD_CLK  13
#define LCD_DIN  11
#define LCD_DC   7
#define LCD_CS   6
#define LCD_RST  5

Display::Display() : lcd(LCD_CLK, LCD_DIN, LCD_DC, LCD_CS, LCD_RST) {
    currentState = STATUS_SCREEN;
}

void Display::begin() {
    lcd.begin();
    lcd.setContrast(50);
    lcd.clearDisplay();
    lcd.setTextSize(1);
    lcd.setTextColor(BLACK);
    clear();
}

void Display::clear() {
    lcd.clearDisplay();
    lcd.display();
}

void Display::update() {
    lcd.display();
}

void Display::drawHeader(const char* title) {
    lcd.drawFastHLine(0, 8, 84, BLACK);
    lcd.setCursor((84 - strlen(title) * 6) / 2, 0);
    lcd.print(title);
}

void Display::showBatteryPresets() {
    lcd.clearDisplay();
    drawHeader("Battery Type");
    lcd.setCursor(0, 12);
    lcd.println("1S LiPo  4.2V");
    lcd.println("2S LiPo  8.4V");
    lcd.println("3S LiPo 12.6V");
    lcd.println("LiFePO4  3.6V");
    lcd.println("Li-HV    4.35V");
    lcd.display();
}

void Display::showError(const char* message) {
    lcd.clearDisplay();
    drawHeader("ERROR");
    lcd.setCursor(0, 12);
    lcd.println(message);
    lcd.display();
}

void Display::drawProgressBar(int x, int y, int width, int height, int percentage) {
    lcd.drawRect(x, y, width, height, BLACK);
    int fillWidth = ((width - 2) * percentage) / 100;
    lcd.fillRect(x + 1, y + 1, fillWidth, height - 2, BLACK);
}
void Display::showPresetMenu(const BatteryPreset* presets, uint8_t count) {
    lcd.clearDisplay();
    drawHeader("Battery Presets");
    
    for (uint8_t i = 0; i < count; i++) {
        lcd.setCursor(0, 10 + i * 8);
        lcd.print(presets[i].name);
        lcd.print(" ");
        lcd.print(presets[i].voltage);
        lcd.print("V");
    }
    lcd.display();
}

void Display::showPresetEdit(const BatteryPreset& preset) {
    lcd.clearDisplay();
    drawHeader("Edit Preset");
    lcd.setCursor(0, 10);
    lcd.print(preset.name);
    lcd.setCursor(0, 20);
    lcd.print("V: ");
    lcd.print(preset.voltage, 2);
    lcd.print("V");
    lcd.setCursor(0, 30);
    lcd.print("I: ");
    lcd.print(preset.current, 2);
    lcd.print("A");
    lcd.display();
}
                               void Display::showDetailedStatus(float voltage, float current, float setVoltage, float setCurrent,
                               uint8_t state, float mAh, unsigned long elapsedTime) {
    lcd.clearDisplay();
    lcd.setCursor(0,0);
    
    // Show charging state
    switch(state) {
        case 0: lcd.print("IDLE"); break;
        case 1: lcd.print("CC"); break;
        case 2: lcd.print("CV"); break;
        case 3: lcd.print("DONE"); break;
        case 4: lcd.print("ERROR"); break;
    }
    lcd.setCursor(48,0);
    char timeStr[9];
    formatTime(elapsedTime, timeStr);
    lcd.print(timeStr);
    
    lcd.setCursor(0,10);
    lcd.print(voltage, 2);
    lcd.print("V ");
    lcd.print(current, 2);
    lcd.print("A");
    
    lcd.setCursor(0,20);
    lcd.print("Set:");
    lcd.print(setVoltage, 1);
    lcd.print("V ");
    lcd.print(setCurrent, 1);
    lcd.print("A");
    
    lcd.setCursor(0,30);
    lcd.print("Charged: ");
    lcd.print(mAh, 0);
    lcd.print("mAh");
    
    // Draw battery icon with safety check
    int percentage = (voltage > 0 && setVoltage > 0) ? 
                    min(100, (int)((voltage / setVoltage) * 100)) : 0;
    drawBatteryIcon(84-15, 0, percentage);
    
    lcd.display();
}

void Display::drawBatteryIcon(int x, int y, int percentage) {
    lcd.drawRect(x, y, 12, 6, BLACK);
    lcd.drawRect(x+12, y+1, 2, 4, BLACK);
    lcd.fillRect(x+1, y+1, (percentage * 10) / 100, 4, BLACK);
}

void Display::formatTime(unsigned long ms, char* buffer) {
    unsigned long seconds = ms / 1000;
    int hours = seconds / 3600;
    int mins = (seconds % 3600) / 60;
    sprintf(buffer, "%02d:%02d", hours, mins);
}

void Display::showVoltageSet(float voltage) {
    lcd.clearDisplay();
    lcd.setCursor(0,0);
    lcd.println("Set Voltage");
    lcd.println("-------------");
    lcd.setTextSize(2);
    lcd.print(voltage, 2);
    lcd.println("V");
    lcd.setTextSize(1);
    lcd.display();
}

void Display::showCurrentSet(float current) {
    lcd.clearDisplay();
    lcd.setCursor(0,0);
    lcd.println("Set Current");
    lcd.println("-------------");
    lcd.setTextSize(2);
    lcd.print(current, 2);
    lcd.println("A");
    lcd.setTextSize(1);
    lcd.display();
}