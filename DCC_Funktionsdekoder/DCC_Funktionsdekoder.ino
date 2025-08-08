#include "DccPacketHandler.h"
#include "outputController.h"
#include "CV_default_values.h"

//DccPacketHandler dcc;
OutputController outputController;


// TODO
// - consist Adresse
// - Neon
// - fadeBlink
// - fade Wechselblink
// - wechselblinken
// - doppelblitz(?)
// - 
//
// zum Code sparen:
// - alle pins fest vergeben und nicht parametrierbar machen

void setup() {
  DccPacketHandler::begin();
  outputController.begin();
  outputController.readCVs();
}

void loop() {
  
  DccPacketHandler::run();
  if(DccPacketHandler::hasUpdate) {
    DccPacketHandler::hasUpdate = false;
    outputController.update(DccPacketHandler::direction, DccPacketHandler::speed, DccPacketHandler::functions);
  }
  outputController.run();
}
