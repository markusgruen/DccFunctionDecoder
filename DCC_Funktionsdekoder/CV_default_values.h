#ifndef CV_DEFAULT_VALUES_H
#define CV_DEFAULT_VALUES_H

#pragma once
#include <Arduino.h>


struct CVDefaults {
  uint8_t address;  // CV-Nummer (CV1 = 1, CV29 = 29, etc.)
  uint8_t value;    // Wert, der in das EEPROM geschrieben werden soll
};

extern const CVDefaults defaultCVs[];
extern const uint8_t numDefaultCVs;

#endif