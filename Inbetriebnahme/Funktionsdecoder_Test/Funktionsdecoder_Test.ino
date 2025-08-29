#include "DccPacketHandler.h"
#include "outputController.h"

//  YOU '''MUST''' BURN THE BOOTLOADER ONCE
//  OTHERWISE THE EEPROM WILL GET ERASED
//  WHENEVER YOU FLASH THE SOFTWARE !!!


void setup() {
  OutputController::begin();
  DccPacketHandler::begin();

  // DccPacketHandler::resetCVsToDefault();
  OutputController::readCVs();
}

void loop() {
  // uint32_t functions = 0b00000000000000000000000000000010;
  // OutputController::update(FORWARD, 10, functions);
  // while(1) {
  //   OutputController::run();
  // }


  DccPacketHandler::run();
  if(DccPacketHandler::hasUpdate) {
    DccPacketHandler::hasUpdate = false;
    OutputController::update(DccPacketHandler::direction, DccPacketHandler::speed, DccPacketHandler::functions);
  }
  
  OutputController::run();

}
