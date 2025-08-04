#include "DccPacketHandler.h"
#include "functionOutputController.h"
#include <EEPROM.h>
// #include "i2cDebugger.h"
// #include "DebugBitBang.h"
#include "debug_uart.h"

FunctionOutputController outputController;
DccPacketHandler dcc;


void setup() {
  pinMode(PIN_PA5, OUTPUT);
  digitalWrite(PIN_PA5, HIGH);
  pinMode(PIN_PA1, OUTPUT);

  debug_uart_init();
  debug_uart_print("TEST");
  
  dcc.begin(4, defaultCVs, numDefaultCVs);
  dcc.resetCVsToDefault();
  uint8_t value = EEPROM.read(29);
  debug_uart_write(value);
  
  // put your setup code here, to run once:
  outputController.begin(PIN7, PIN2, PIN3, PIN6);
  outputController.readCVs();

}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t functions = 0b00000000000000000000000000000010;
  outputController.update(FORWARD, 10, functions);
  while(1);
}
