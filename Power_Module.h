#ifndef POWER_MODULE_H
#define POWER_MODULE_H

#include <Arduino.h>
#include <Wire.h>

#define INA226_ADDR 0x40

volatile float busVoltage_V = 0;
volatile float shuntVoltage_mV = 0;
volatile float current_mA = 0;
volatile float power_mW = 0;
volatile bool  ina226_online = false;

// Rolling history for graphs (230 pixels wide on TFT)
#define POWER_HIST_LEN 230
float voltHist[POWER_HIST_LEN];
float currHist[POWER_HIST_LEN];
float pwrHist[POWER_HIST_LEN];

uint16_t ina226_read16(uint8_t reg) {
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)INA226_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    uint16_t val = Wire.read() << 8;
    val |= Wire.read();
    return val;
  }
  return 0;
}

bool ina226_init() {
  Wire.beginTransmission(INA226_ADDR);
  if (Wire.endTransmission() != 0) {
    ina226_online = false;
    return false;
  }
  
  // Configuration Register 0x00
  // Default is 0x4127 (Averages=1, VBUS 1.1ms, VSHUNT 1.1ms, Continuous Shunt & Bus)
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(0x00);
  Wire.write(0x41); // MSB
  Wire.write(0x27); // LSB
  Wire.endTransmission();
  ina226_online = true;
  
  for(int i=0; i<POWER_HIST_LEN; i++){
    voltHist[i] = 0;
    currHist[i] = 0;
    pwrHist[i] = 0;
  }
  return true;
}

void ina226_update() {
  if (!ina226_online) return;
  
  int16_t vbus_raw = ina226_read16(0x02);
  busVoltage_V = vbus_raw * 0.00125f;
  
  int16_t vshunt_raw = ina226_read16(0x01);
  shuntVoltage_mV = vshunt_raw * 0.0025f; // 2.5uV per LSB
  
  // Assuming a standard 0.1 Ohm (R100) shunt resistor (common on cheap modules)
  // If it's R010 (0.01 Ohm), change 0.1f to 0.01f
  current_mA = shuntVoltage_mV / 0.1f; 
  power_mW = busVoltage_V * current_mA;

  // shift history
  for(int i=0; i<POWER_HIST_LEN-1; i++){
    voltHist[i] = voltHist[i+1];
    currHist[i] = currHist[i+1];
    pwrHist[i]  = pwrHist[i+1];
  }
  voltHist[POWER_HIST_LEN-1] = busVoltage_V;
  currHist[POWER_HIST_LEN-1] = current_mA;
  pwrHist[POWER_HIST_LEN-1]  = power_mW;
}

#endif
