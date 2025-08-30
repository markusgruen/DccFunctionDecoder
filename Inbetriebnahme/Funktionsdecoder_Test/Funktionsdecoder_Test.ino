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

  DccPacketHandler::run();
  if(DccPacketHandler::hasUpdate) {
    DccPacketHandler::hasUpdate = false;
    OutputController::update(DccPacketHandler::direction, DccPacketHandler::functions);
  }
  
  OutputController::run();

}
