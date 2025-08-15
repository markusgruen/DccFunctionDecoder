#include "pinmap.h"
#include "DccSignalReceiver_POLLING.h"


void setup() {
  // put your setup code here, to run once:
  
  // Pin als Eingang 
  PORTA.DIRSET = CH1_PIN;
  // PORTA.OUTSET = CH1_PIN;

  DccSignalReceiver::begin();



}

void loop() {
  // // put your main code here, to run repeatedly:
  // PORTA.OUTSET = LED;
  // delay(500);
  // PORTA.OUTCLR = LED;
  // delay(500);

}
