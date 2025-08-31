#include "DccPacketHandler.h"
#include "outputController.h"

//  YOU '''MUST''' BURN THE BOOTLOADER ONCE
//  OTHERWISE THE EEPROM WILL GET ERASED
//  WHENEVER YOU FLASH THE SOFTWARE !!!


void setup() {
  OutputController::begin();
  DccPacketHandler::begin();
  OutputController::readCVs();
}

void loop() {

  DccPacketHandler::run();
  OutputController::run(DccPacketHandler::direction, DccPacketHandler::functions);

}
