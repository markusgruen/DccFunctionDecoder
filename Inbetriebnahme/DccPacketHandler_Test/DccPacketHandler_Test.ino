#include "DccPacketHandler.h"
#include "pinmap.h"

void setup() {
  // put your setup code here, to run once:
  
  // Pin als Eingang 
  PORTA.DIRSET = CH1_PIN;
  PORTA.OUTSET = CH1_PIN;

  DccPacketHandler::resetCVsToDefault();
  DccPacketHandler::begin();
}

void loop() {
  DccPacketHandler::run();


}
