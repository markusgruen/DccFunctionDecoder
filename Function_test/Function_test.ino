#include "DccPacketHandler.h"
#include "functionOutputController.h"

FunctionOutputController outputController;


void setup() {
  // put your setup code here, to run once:
  outputController.begin(PIN1, PIN3, PIN6, PIN7);
  outputController.readCVs();
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t functions = 0b00000000000000000000000000000010
  outputController.update(FORWARD, 10, dcc.getFunctions());
  while(1);
}
