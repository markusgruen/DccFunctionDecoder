#include "DccSignalReceiver.h"
#include <Arduino.h>
#include <avr/interrupt.h>

/*
constexpr uint32_t US_TO_TICKS(uint32_t us) {
    return (us * (F_CPU / 1000000UL));
}

constexpr uint16_t DCC_MIN0_TICKS = US_TO_TICKS(52);
constexpr uint16_t DCC_MAX0_TICKS = US_TO_TICKS(64);
constexpr uint16_t DCC_MIN1_TICKS = US_TO_TICKS(100);
constexpr uint16_t DCC_MAX1_TICKS = US_TO_TICKS(200);



volatile int8_t vLastHalfBit = -1;
volatile uint64_t vBitstream = 0;
volatile uint8_t vNumReceivedBits = 0;
*/


DccSignalReceiver::DccSignalReceiver() {
}

void DccSignalReceiver::begin(uint8_t pin) {
/*  // Pin als Eingang mit Pull-Up, falls nötig
  pinMode(pin, INPUT);

  // TCB0 auf Event-Capture-Modus einstellen:
  // - Count Mode: Capture (TCB_CNTMODE_CAPTURE_gc)
  // - Capture/Compare Enable (TCB_CCMPEN_bm)
  // - Capture Event Interrupt Enable (TCB_CAPTEI_bm)
  // - Clock Source: CLK_PER (div 1)

  // 1. Event system konfigurieren, Signal an TCB0 als Eventquelle koppeln
  // Eventchannel 0 mit Pin PA2 verbinden (Event Channel 0, PORTA Pin 2)
  // ATtiny402 Event-System:
  EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN2_gc;
  EVSYS.USER0 = EVSYS_USER0_TCB0_CAPT_gc; // TCB0 Capture als User0

  // 2. TCB0 konfigurieren
  TCB0.CTRLB = TCB_CNTMODE_CAPTURE_gc | TCB_CCMPEN_bm; // Capture Mode, CCMP Enable
  TCB0.EVCTRL = TCB_CAPTEI_bm;                         // Capture Interrupt Enable via Event
  TCB0.INTCTRL = TCB_CAPT_bm;                          // Interrupt bei Capture
  TCB0.CNT = 0;                                        // Timer zählen von 0
  TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;    // Taktquelle CLK_PER, Timer Enable
*/
}

void DccSignalReceiver::getBitstream(uint64_t* bitstream, uint8_t* numReceivedBits) {
  /*noInterrupts();
  *numReceivedBits = vNumReceivedBits;
  *bitstream = vBitstream;
  vNumReceivedBits = 0;
  vBitstream = 0;
  interrupts();
  */
  *bitstream = 0b10000111101111000001111111111111101110UL;
  *numReceivedBits = 38;
}

/*
ISR(TCB0_INT_vect) {
  uint16_t pulseLength = TCB0.CCMP;  // Timerstand kopieren
  TCB0.CNT = 0;                     // Timer zurücksetzen
  
  // check if the current half-bit is 0, 1 or invalid
  int thisHalfBit = -1;
  if(pulseLength >= DCC_MIN0_TICKS && pulseLength <= DCC_MAX0_TICKS) {
    thisHalfBit = 0ULL;
  }
  else if (pulseLength >= DCC_MIN1_TICKS && pulseLength <= DCC_MAX1_TICKS) {
    thisHalfBit = 1ULL;
  }
  
  // build the Bitstream if two consecutive half-bits are equal
  if (vLastHalfBit == -1) {
    // if this is the first half-bit
    vLastHalfBit = thisHalfBit;
  }
  else {
    // if this is the second half-bit
    if (thisHalfBit == vLastHalfBit) {
      vBitstream |= (thisHalfBit << vNumReceivedBits);
      vNumReceivedBits++;
    }
    else {
      vBitstream = 0ULL;
      vNumReceivedBits = 0;
    }

    vLastHalfBit = -1; // immer zurücksetzen nach einem Paar
  }
}

*/