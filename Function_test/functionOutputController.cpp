#include "functionOutputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "debug_uart.h"

volatile uint8_t FunctionOutputController::pwmValues[NUM_CHANNELS] = {0};
uint8_t FunctionOutputController::mChannelPin_bm[NUM_CHANNELS] = {0};


FunctionOutputController::FunctionOutputController(){

  // DELETE
  mChannelPinState[0] = OFF;
  mChannelPinState[1] = OFF;
  mChannelPinState[2] = OFF;
  mChannelPinState[3] = OFF;

  mMode[0] = ONOFF;
  mMode[1] = ONOFF;
  mMode[2] = ONOFF;
  mMode[3] = ONOFF;
}

void FunctionOutputController::begin(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4){

  // setup Timer A for software-PWM with >100Hz frequency at 8bit resolution
  // 20 MHz / 8 = 2.5 MHz → 1 Takt = 0.4 µs → 30 µs = 75 Takte
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV8_gc;   // Clock = 20 MHz / 8 = 2.5 MHz
  TCA0.SINGLE.PER = 74;                            // Compare match nach 75 Takten = 30 µs
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;        // Compare match interrupt aktivieren
  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;       // Timer aktivieren

  // configure output pins
  mChannelPin_bm[0] = (1 << pin1);
  mChannelPin_bm[1] = (1 << pin2);
  mChannelPin_bm[2] = (1 << pin3);
  mChannelPin_bm[3] = (1 << pin4);

  PORTA.DIRSET |= mChannelPin_bm[0];
  PORTA.DIRSET |= mChannelPin_bm[1];
  PORTA.DIRSET |= mChannelPin_bm[2];
  PORTA.DIRSET |= mChannelPin_bm[3];
}

void FunctionOutputController::readCVs() {
  mFunctionMap[0]  = (uint32_t)EEPROM.read(ch1FunctionMap0);
  mFunctionMap[0] |= (uint32_t)EEPROM.read(ch1FunctionMap1) << 8;
  mFunctionMap[0] |= (uint32_t)EEPROM.read(ch1FunctionMap2) << 16;

  mFunctionMap[1]  = (uint32_t)EEPROM.read(ch2FunctionMap0);
  mFunctionMap[1] |= (uint32_t)EEPROM.read(ch2FunctionMap1) << 8;
  mFunctionMap[1] |= (uint32_t)EEPROM.read(ch2FunctionMap2) << 16;

  mFunctionMap[2]  = (uint32_t)EEPROM.read(ch3FunctionMap0);
  mFunctionMap[2] |= (uint32_t)EEPROM.read(ch3FunctionMap1) << 8;
  mFunctionMap[2] |= (uint32_t)EEPROM.read(ch3FunctionMap2) << 16;

  mFunctionMap[3]  = (uint32_t)EEPROM.read(ch4FunctionMap0);
  mFunctionMap[3] |= (uint32_t)EEPROM.read(ch4FunctionMap1) << 8;
  mFunctionMap[3] |= (uint32_t)EEPROM.read(ch4FunctionMap2) << 16;

  mSpeedThreshold[0] = EEPROM.read(ch1SpeedThreshold);
  mSpeedThreshold[1] = EEPROM.read(ch2SpeedThreshold);
  mSpeedThreshold[2] = EEPROM.read(ch3SpeedThreshold);
  mSpeedThreshold[3] = EEPROM.read(ch4SpeedThreshold);

  mDimmValue[0] = EEPROM.read(ch1DimmValue);
  mDimmValue[1] = EEPROM.read(ch2DimmValue);
  mDimmValue[2] = EEPROM.read(ch3DimmValue);
  mDimmValue[3] = EEPROM.read(ch4DimmValue);

  mMode[0] = (Mode)EEPROM.read(ch1Mode1);
  mMode[1] = (Mode)EEPROM.read(ch2Mode1);
  mMode[2] = (Mode)EEPROM.read(ch3Mode1);
  mMode[3] = (Mode)EEPROM.read(ch4Mode1);
}

void FunctionOutputController::update(Direction direction, uint8_t speed, uint32_t functions) {
  for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
    bool channelIsOn = isChannelOn(functions, channel);
    bool directionMatches = directionMatchesConfig(channel, direction);
    bool speedBelowThreshold = IsSpeedBelowLimit(channel, speed);

    if(channelIsOn && directionMatches && speedBelowThreshold) {
      switchOn(channel);
    } else {
      switchOff(channel);
    }
  }
}

void FunctionOutputController::switchOn(uint8_t channel) {
  if(mChannelPinState[channel] == OFF || mChannelPinState[channel] == TRANSITION_OFF) {
    switch(mMode[channel]) {
      case ONOFF:
        pwmValues[channel] = mDimmValue[channel];
        mChannelPinState[channel] = ON;  
        break;
    }
  }
}

void FunctionOutputController::switchOff(uint8_t channel) {
  if(mChannelPinState[channel] == ON || mChannelPinState[channel] == TRANSITION_ON) {
    switch(mMode[channel]) {
      case ONOFF:
        pwmValues[channel] = 0;
        mChannelPinState[channel] = OFF;  
        break;
    }
  }
}

ISR(TCA0_CMP0_vect) {
  static uint8_t sPwmCounter = 0;

  for (uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
    if (sPwmCounter < FunctionOutputController::pwmValues[channel]) {
      PORTA.OUTSET = FunctionOutputController::mChannelPin_bm[channel];  // High
    } else {
      PORTA.OUTCLR = FunctionOutputController::mChannelPin_bm[channel];  // Low
    }
  }
  sPwmCounter++;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; // Interrupt-Flag löschen
}