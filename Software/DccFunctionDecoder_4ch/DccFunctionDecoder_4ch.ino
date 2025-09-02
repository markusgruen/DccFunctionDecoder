#include "DccPacketHandler.h"
#include "outputController.h"

/******************************************************
  This is a DCC function decoder with 4 channel outputs
  --> https://github.com/markusgruen/DccFunctionDecoder

  Copyright (c) 2025 Markus Grün
  GNU General Public License v3.0
*****************************************************/

// 1. install megaTinyCore: https://github.com/SpenceKonde/megaTinyCore
// 2. select Tools -> Board -> megaTinyCore - >ATtiny412/402/212/202
// 3. select Tools -> Chip -> ATtiny 402
// 4. select Tools -> millis()/micros() Timer: "Disabled"
// 5. connect UART adapter with the decoder as shown in the documentation. Don't forget to switch on power to the rails.
// 6. select Tools -> Port -> the UART adapter's COM-port
// 7. select Tools -> Progrmamer -> SerialUPDI - 230400 Baud
// 8. select Tools -> Burn Bootloader  <-- you must do this once, otherwise EEPROM will get erased whenever you flash the software


void setup() {

  OutputController::begin();
  DccPacketHandler::begin();
  OutputController::readCVs();

}

void loop() {

  DccPacketHandler::run();
  OutputController::run(DccPacketHandler::direction, DccPacketHandler::functions);

}
