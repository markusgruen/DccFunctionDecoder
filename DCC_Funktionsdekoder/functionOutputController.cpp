#include "functionOutputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>

FunctionOutputController::FunctionOutputController(){

}

void FunctionOutputController::begin(){
  mChannelPin_bm[0] = PIN1_bm;
  mChannelPin_bm[1] = PIN3_bm;
  mChannelPin_bm[2] = PIN6_bm;
  mChannelPin_bm[3] = PIN7_bm;

  PORTA.DIRSET = mChannelPin_bm[0];
  PORTA.DIRSET = mChannelPin_bm[1];
  PORTA.DIRSET = mChannelPin_bm[2];
  PORTA.DIRSET = mChannelPin_bm[3];
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

}

void FunctionOutputController::update(Direction direction, uint8_t speed, uint32_t functions) {
  bool willBeSwitchedOn = false;
  bool willBeSwitchedOff = false;

  for(uint8_t channel=0; channel<4; channel++) {

    // switch off channels if function is deactivated
    // what happens, if one channel is attached to multiple functions?
    if( (switched_off(mFunctions, functions) & mFunctionMap[channel])  // if channel was switched off...
        && ~(functions & mFunctionMap[channel])                        // ...and no other function keeps it on, ...
        && (mChannelPinState[channel] == ON || mChannelPinState[channel] == TRANSITION_ON) ) {       // ...and channel is not already off
      willBeSwitchedOff = true;
    }

    // switch on channels if function is activated
    if( (switched_on(mFunctions,functions) & mFunctionMap[channel])  // if channel was switched on...
        && (mChannelPinState[channel] == OFF || mChannelPinState[channel] == TRANSITION_OFF)       // ...and channel ist not already on, ...
        && ((forwardOnly(channel) && direction == FORWARD) || (reverseOnly(channel) && direction == REVERSE)) ) {   
      willBeSwitchedOn = true;
    }
    
    // switch channels that depend on direction
    if( forwardOnly(channel) && mDirection == REVERSE && direction == FORWARD) {
      willBeSwitchedOn = true;
    }
    if( reverseOnly(channel) && mDirection == FORWARD && direction == REVERSE) {
      willBeSwitchedOn = true;
    }
    if( forwardOnly(channel) && mDirection == FORWARD && direction == REVERSE) {
      willBeSwitchedOff = true;
    }
    if( reverseOnly(channel) && mDirection == REVERSE && direction == FORWARD) {
      willBeSwitchedOff = true;
    }


    if(willBeSwitchedOn && !willBeSwitchedOff) {
      switchOn(channel);
    }
    else if (!willBeSwitchedOn & willBeSwitchedOff) {
      switchOff(channel);
    }
  }
}

void FunctionOutputController::switchOn(uint8_t channel) {
  // TODO hier die unterschiedlichen anschaltcharakteristiken abprüfen und entsprechend ausführen
  PORTA.OUTCLR = channel;  // LOW
  mChannelPinState[channel] = ON;
}

void FunctionOutputController::switchOff(uint8_t channel) {
  PORTA.OUTSET = channel;  // HIGH
  mChannelPinState[channel] = OFF;
}