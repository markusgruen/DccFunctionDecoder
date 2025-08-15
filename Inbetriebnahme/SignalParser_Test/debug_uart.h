#pragma once
#include <avr/io.h>
#include <util/delay.h>

#define DEBUG_TX_PORT PORTA
#define DEBUG_TX_PIN  PIN1_bm  // PA1 z. B.

void debug_uart_init();
void debug_uart_write(uint8_t byte);
void debug_uart_print(const char* s);
