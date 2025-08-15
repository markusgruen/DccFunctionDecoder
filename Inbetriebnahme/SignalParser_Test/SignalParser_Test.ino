#include "DccSignalParser.h"
#include "pinmap.h"
#include "debug_uart.h"


void setup() {
  // put your setup code here, to run once:
  
  // Pin als Eingang 
  PORTA.DIRSET = CH1_PIN;
  PORTA.OUTCLR = CH1_PIN;

  DccSignalParser::begin();
  // debug_uart_init();
  // debug_uart_write(0x42);


}

void loop() {
  DccSignalParser::run();

}
