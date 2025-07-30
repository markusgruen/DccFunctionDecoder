#include "DccPacketHandler.h"
#include "functionOutputController.h"

DccPacketHandler dcc;
FunctionOutputController outputController;


// funktionen: an-/ausschalten mit verschiedenen Modi (fading, flacker, blinken, etc)
// funktionen fahrtrichtungsabhängig schalten
// CVs im EEProm abspeichern
// 


void setup() {
  dcc.begin(PIN2_bm);
  outputController.begin();
  outputController.readCVs();
}

void loop() {
  
  dcc.run();
  if(dcc.hasUpdate()) {
    outputController.update(dcc.getDirection(), dcc.getSpeed(), dcc.getFunctions());
  }
}
