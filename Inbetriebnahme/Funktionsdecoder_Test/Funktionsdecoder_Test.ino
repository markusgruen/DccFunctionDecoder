#include "DccPacketHandler.h"
#include "outputController.h"


void setup() {
  // uint8_t dimm[4] = {127, 127, 127, 127};
  // uint8_t fade[4] = {50, 50, 50, 50};
  // uint8_t blinkOn[4] = {50, 50, 50, 50};
  // uint8_t blinkOff[4] = {150, 150, 150, 150};
  // Mode m[4]= {FADE, FADE, FADE, FADE};

  // memcpy(OutputController::dimmValue, dimm, 4);
  // memcpy(OutputController::fadeSpeed, fade, 4);
  // memcpy(OutputController::blinkOnTime, blinkOn, 4);
  // memcpy(OutputController::blinkOffTime, blinkOff, 4);
  // memcpy(OutputController::mode[NUM_CHANNELS], m, 4);
  // Direction direction = FORWARD;
  // State channelPinState[4];  




  // DccPacketHandler::resetCVsToDefault();
  
  // DccPacketHandler::begin();
  OutputController::begin();
  // OutputController::readCVs();

}

void loop() {
  uint32_t functions = 0b00000000000000000000000000000010;
  OutputController::update(FORWARD, 10, functions);
  while(1) {
    OutputController::run();
  }


  // DccPacketHandler::run();
  // if(DccPacketHandler::hasUpdate) {
    // DccPacketHandler::hasUpdate = false;
    // OutputController::update(DccPacketHandler::direction, DccPacketHandler::speed, DccPacketHandler::functions);
  // }
  // OutputController::run();

}
