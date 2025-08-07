#include "DccPacketHandler.h"
#include "outputController.h"
#include "CV_default_values.h"

DccPacketHandler dcc;
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
  dcc.begin(defaultCVs, numDefaultCVs);
  outputController.begin();
  outputController.readCVs();
}

void loop() {
  
  dcc.run();
  if(dcc.hasUpdate()) {
    outputController.update(dcc.getDirection(), dcc.getSpeed(), dcc.getFunctions());
  }
  outputController.run();
}
