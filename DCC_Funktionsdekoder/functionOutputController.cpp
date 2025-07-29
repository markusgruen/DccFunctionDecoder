#include "functionOutputController.h"
#include "CV_table.h"
#include <EEPROM.h>

FunctionOutputController::FunctionOutputController(){

}

void FunctionOutputController::begin(){
  pinMode(PIN_PA1, OUTPUT);
  pinMode(PIN_PA3, OUTPUT);
  pinMode(PIN_PA6, OUTPUT);
  pinMode(PIN_PA7, OUTPUT);
}

void FunctionOutputController::readCVs() {
  mFunctionMap[0]  = EEPROM.read(ch1FunctionMap0);
  mFunctionMap[0] |= EEPROM.read(ch1FunctionMap1) << 8;
  mFunctionMap[0] |= EEPROM.read(ch1FunctionMap2) << 16;
}

void FunctionOutputController::update(Direction direction, uint8_t speed, uint32_t functions) {
  
  for(uint8_t iFunction=0; iFunction<28; iFunction++) {
    for(uint8_t iChannel=0; iChannel<4; iChannel++) {
      if(bit_is_set(mFunctionMap[iChannel], iFunction) {
        if(bit_switched_on(mFunctions, functions, iFunction)) {
          switch_on(iChannel);
      }
      }

      
    }
  }
}