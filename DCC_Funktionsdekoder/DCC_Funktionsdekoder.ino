#include "DccPacketHandler.h"
#include "functionOutputController.h"
#include "CV_default_values.h"

DccPacketHandler dcc;
FunctionOutputController outputController;



// funktionen: an-/ausschalten mit verschiedenen Modi (fading, flacker, blinken, etc)
// funktionen fahrtrichtungsabhängig schalten
// CVs im EEProm abspeichern
// 


void setup() {
  dcc.begin(PIN2, defaultCVs, numDefaultCVs);
  outputController.begin(PIN1, PIN3, PIN6, PIN7);
  outputController.readCVs();
}

void loop() {
  
  dcc.run();
  if(dcc.hasUpdate()) {
    outputController.update(dcc.getDirection(), dcc.getSpeed(), dcc.getFunctions());
  }
  outputController.run();
}
