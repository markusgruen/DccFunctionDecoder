#include "DccPacketParser.h"

DccPacketParser dcc;


// funktionen: an-/ausschalten mit verschiedenen Modi (fading, flacker, blinken, etc)
// funktionen fahrtrichtungsabhängig schalten
// CVs im EEProm abspeichern
// 


void setup() {
  dcc.begin(PIN_PA2);
}

void loop() {
  

  dcc.run();
  if(dcc.hasUpdate()) {
    dcc.getDirection();
    //dcc.getFunctions();
  }
}
