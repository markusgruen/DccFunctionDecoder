#include "debug_uart.h"

// 9600 Baud = 104 µs pro Bit
#define BIT_DELAY_US 52

void debug_uart_init() {
  DEBUG_TX_PORT.DIRSET = DEBUG_TX_PIN;  // Ausgang
  DEBUG_TX_PORT.OUTSET = DEBUG_TX_PIN;  // Idle high
}

void debug_uart_write(uint8_t byte) {
  // Startbit (LOW)
  DEBUG_TX_PORT.OUTCLR = DEBUG_TX_PIN;
  _delay_us(BIT_DELAY_US);

  // 8 Datenbits, LSB first
  for (uint8_t i = 0; i < 8; i++) {
    if (byte & (1 << i))
      DEBUG_TX_PORT.OUTSET = DEBUG_TX_PIN;
    else
      DEBUG_TX_PORT.OUTCLR = DEBUG_TX_PIN;

    _delay_us(BIT_DELAY_US);
  }

  // Stopbit (HIGH)
  DEBUG_TX_PORT.OUTSET = DEBUG_TX_PIN;
  _delay_us(BIT_DELAY_US);
}

void debug_uart_print(const char* s) {
  while (*s) {
    debug_uart_write(*s++);
  }
}
