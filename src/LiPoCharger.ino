// src/LiPoCharger.ino
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <U8g2lib.h>
#include <EEPROM.h>
#include <Encoder.h>
#include <avr/wdt.h>

// Pin Definitions
#define PWM_PIN 9    // PWM to LM2596 FB
#define MOSFET_PIN 6 // Load switch for resistance
#define BUZZER_PIN 7 // Buzzer
#define ENC_CLK 2    // Encoder CLK
#define ENC_DT 3     // Encoder DT
#define ENC_SW 4     // Encoder Switch

// Objects
Adafruit_INA219 ina219;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
Encoder encoder(ENC_CLK, ENC_DT);

// Variables
float voltage = 0, current = 0, v_oc = 0, v_load = 0, i_load = 0, r_int = 0;
float charged_mAh = 0, soh = 100;
float cal_voltage_factor = 1.0, cal_current_factor = 1.0;
int mode = 0; // 0: Home, 1: Charge, 2: Resistance, 3: Settings, 4: Calibrate
int menu_pos = 0, preset = 0, cal_step = 0;
float cal_voltage_ref = 0, cal_current_ref = 0;
int pwm_value = 0;

// Buzzer Alert Types
#define BUZZ_TICK 0
#define BUZZ_PUSH 1
#define BUZZ_CHARGE 2
#define BUZZ_RESIST 3
#define BUZZ_CAL_STEP 4
#define BUZZ_CAL_DONE 5
#define BUZZ_FAULT 6
#define BUZZ_FULL 7

struct Battery {
  float capacity, nom_voltage, power;
  float r_new, r_max;
} presets[4];

// EEPROM Addresses
#define CAL_V_ADDR 128
#define CAL_C_ADDR 132

void setup() {
  pinMode(PWM_PIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENC_SW, INPUT_PULLUP);
  
  Wire.begin();
  ina219.begin();
  oled.begin();
  loadPresets();
  loadCalibration();
  
  analogWrite(PWM_PIN, 0);
  wdt_enable(WDTO_8S);
}

void loop() {
  wdt_reset();
  readSensors();
  checkSafety();
  updateDisplay();
  handleEncoder();
  
  if (mode == 1) chargeBattery();
  if (mode == 2) calcResistance();
  if (mode == 4) calibrateINA219();
}

void readSensors() {
  voltage = ina219.getBusVoltage_V() * cal_voltage_factor;
  current = ina219.getCurrent_mA() * cal_current_factor;
  if (mode == 1 && current > 0) charged_mAh += (current * 0.0167);
}

void chargeBattery() {
  float target_voltage = presets[preset].nom_voltage + 0.5;
  float target_current = presets[preset].capacity * 0.5;
  
  if (voltage < target_voltage - 0.1) {
    if (current < target_current) pwm_value = constrain(pwm_value + 1, 0, 255);
    else if (current > target_current + 50) pwm_value = constrain(pwm_value - 1, 0, 255);
  } else {
    if (voltage > target_voltage) pwm_value = constrain(pwm_value - 1, 0, 255);
    else if (voltage < target_voltage - 0.05) pwm_value = constrain(pwm_value + 1, 0, 255);
  }
  
  analogWrite(PWM_PIN, pwm_value);
  if (voltage >= target_voltage && current < 50) {
    analogWrite(PWM_PIN, 0);
    buzzerAlert(BUZZ_FULL);
    mode = 0;
  }
}

void calcResistance() {
  digitalWrite(MOSFET_PIN, LOW);
  delay(100);
  v_oc = ina219.getBusVoltage_V() * cal_voltage_factor;
  digitalWrite(MOSFET_PIN, HIGH);
  delay(100);
  v_load = ina219.getBusVoltage_V() * cal_voltage_factor;
  i_load = (ina219.getCurrent_mA() * cal_current_factor) / 1000.0;
  r_int = (v_oc - v_load) / i_load;
  soh = 100 * (presets[preset].r_max - r_int) / (presets[preset].r_max - presets[preset].r_new);
  soh = constrain(soh, 0, 100);
  digitalWrite(MOSFET_PIN, LOW);
  buzzerAlert(BUZZ_RESIST);
  mode = 0;
}

void calibrateINA219() {
  if (cal_step == 0) {
    oled.clearBuffer();
    oled.drawStr(0, 10, "Set Volt Ref (V):");
    oled.drawStr(0, 20, String(cal_voltage_ref, 2).c_str());
    oled.sendBuffer();
    cal_voltage_ref += encoder.read() * 0.01;
    encoder.write(0);
  } else if (cal_step == 1) {
    float raw_v = ina219.getBusVoltage_V();
    cal_voltage_factor = cal_voltage_ref / raw_v;
    oled.clearBuffer();
    oled.drawStr(0, 10, "Set Curr Ref (mA):");
    oled.drawStr(0, 20, String(cal_current_ref, 0).c_str());
    oled.sendBuffer();
    cal_current_ref += encoder.read() * 1.0;
    encoder.write(0);
  } else if (cal_step == 2) {
    float raw_c = ina219.getCurrent_mA();
    cal_current_factor = cal_current_ref / raw_c;
    EEPROM.put(CAL_V_ADDR, cal_voltage_factor);
    EEPROM.put(CAL_C_ADDR, cal_current_factor);
    buzzerAlert(BUZZ_CAL_DONE);
    mode = 0;
    cal_step = 0;
  }
}

void updateDisplay() {
  oled.clearBuffer();
  if (mode == 0) {
    oled.setFont(u8g2_font_ncenB08_tr);
    oled.drawStr(0, 10, "LiPo Charger");
    oled.setFont(u8g2_font_tiny5_tf);
    oled.drawStr(0, 20, ("Volt: " + String(voltage, 2) + "V").c_str());
    oled.drawStr(0, 30, ("Curr: " + String(current, 1) + "mA").c_str());
    oled.drawStr(0, 40, ("Cap: " + String(charged_mAh, 0) + "mAh").c_str());
    oled.drawStr(0, 50, ("Rint: " + String(r_int, 3) + "Ohm").c_str());
    oled.drawStr(0, 60, ("SoH: " + String(soh, 1) + "%").c_str());
  } else if (mode == 3) {
    oled.setFont(u8g2_font_tiny5_tf);
    oled.drawStr(0, 10, "Settings");
    oled.drawStr(0, 20, (menu_pos == 0 ? "> " : "  ") + String("Cap: ") + presets[preset].capacity);
    oled.drawStr(0, 30, (menu_pos == 1 ? "> " : "  ") + String("Volt: ") + presets[preset].nom_voltage);
    oled.drawStr(0, 40, (menu_pos == 2 ? "> " : "  ") + String("Rnew: ") + presets[preset].r_new);
    oled.drawStr(0, 50, (menu_pos == 3 ? "> " : "  ") + String("Rmax: ") + presets[preset].r_max);
    oled.drawStr(0, 60, (menu_pos == 4 ? "> " : "  ") + String("Calibrate"));
  }
  oled.sendBuffer();
}

void handleEncoder() {
  int enc_val = encoder.read();
  if (enc_val != 0) {
    buzzerAlert(BUZZ_TICK);
    if (mode == 0) mode = 3;
    else if (mode == 3) {
      menu_pos = constrain(menu_pos + enc_val / 4, 0, 4);
      if (menu_pos == 0) presets[preset].capacity += enc_val * 100;
      if (menu_pos == 1) presets[preset].nom_voltage += enc_val * 0.1;
      if (menu_pos == 2) presets[preset].r_new += enc_val * 0.001;
      if (menu_pos == 3) presets[preset].r_max += enc_val * 0.01;
    } else if (mode == 4) {
      if (cal_step == 0) cal_voltage_ref += enc_val * 0.01;
      else if (cal_step == 1) cal_current_ref += enc_val * 1.0;
    }
    encoder.write(0);
  }
  if (!digitalRead(ENC_SW)) {
    delay(50);
    buzzerAlert(BUZZ_PUSH);
    if (mode == 0) { mode = 1; buzzerAlert(BUZZ_CHARGE); }
    else if (mode == 3 && menu_pos == 4) { mode = 4; cal_step = 0; }
    else if (mode == 3) savePresets();
    else if (mode == 4) { cal_step++; buzzerAlert(BUZZ_CAL_STEP); }
    else { mode = 0; buzzerAlert(BUZZ_CHARGE); }
  }
}

void loadPresets() {
  for (int i = 0; i < 4; i++) {
    EEPROM.get(i * sizeof(Battery), presets[i]);
    if (presets[i].capacity == 0) {
      presets[i].capacity = 1000;
      presets[i].nom_voltage = 3.7;
      presets[i].power = 3.7;
      presets[i].r_new = 0.02;
      presets[i].r_max = 0.2;
    }
  }
}

void savePresets() {
  EEPROM.put(preset * sizeof(Battery), presets[preset]);
  buzzerAlert(BUZZ_PUSH);
}

void loadCalibration() {
  EEPROM.get(CAL_V_ADDR, cal_voltage_factor);
  EEPROM.get(CAL_C_ADDR, cal_current_factor);
  if (cal_voltage_factor == 0 || cal_current_factor == 0) {
    cal_voltage_factor = 1.0;
    cal_current_factor = 1.0;
  }
}

void checkSafety() {
  if (voltage > 4.3 || voltage < 3.0) {
    analogWrite(PWM_PIN, 0);
    buzzerAlert(BUZZ_FAULT);
    mode = 0;
  }
}

void buzzerAlert(int type) {
  switch (type) {
    case BUZZ_TICK:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(20);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case BUZZ_PUSH:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case BUZZ_CHARGE:
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
      break;
    case BUZZ_RESIST:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case BUZZ_CAL_STEP:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(50);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case BUZZ_CAL_DONE:
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(150);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
      break;
    case BUZZ_FAULT:
      for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);
        delay(50);
      }
      break;
    case BUZZ_FULL:
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(150);
      digitalWrite(BUZZER_PIN, LOW);
      delay(50);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      break;
  }
}
