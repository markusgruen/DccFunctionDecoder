#include "DccSignalReceiver_POLLING.h"
#include <Arduino.h>
#include <avr/interrupt.h>


extern volatile uint8_t DccSignalReceiver::mDccPinMask;

/*
constexpr uint32_t US_TO_TICKS(uint32_t us) {
    return (us * (F_CPU / 1000000UL));
}

constexpr uint16_t DCC_MIN0_TICKS = US_TO_TICKS(52);
constexpr uint16_t DCC_MAX0_TICKS = US_TO_TICKS(64);
constexpr uint16_t DCC_MIN1_TICKS = US_TO_TICKS(100);
constexpr uint16_t DCC_MAX1_TICKS = US_TO_TICKS(200);
*/

#define MITTELWERT 20

volatile uint64_t vBitstream = 0;
volatile uint8_t vNumReceivedBits = 0;


DccSignalReceiver::DccSignalReceiver() {
}

void DccSignalReceiver::begin(uint8_t pinMask) {
  mDccPinMask = pinMask;
  // Pin als Eingang 
  PORTA.DIRCLR = mDccPinMask;


  // CTC (Frequency) Mode
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc;

  // 20 MHz / 4 = 5 MHz → 1 Tick = 0.2 µs
  // 5 µs / 0.2 µs = 25 Ticks → CMP0 = 24
  TCA0.SINGLE.CMP0 = 24;

  // Enable Compare Match Interrupt
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;

  // Start Timer with Prescaler 4
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm;
}

void DccSignalReceiver::getBitstream(uint64_t* bitstream, uint8_t* numReceivedBits) {
  noInterrupts();
  *numReceivedBits = vNumReceivedBits;
  *bitstream = vBitstream;
  vNumReceivedBits = 0;
  vBitstream = 0;
  interrupts();
}

ISR(TCA0_CMP0_vect) {
  static uint8_t vLastHalfBit = 0xFF;
  static uint16_t vNumHigh = 0;
  static uint16_t vNumLow = 0;
  static uint16_t vHighCount = 0;
  static uint16_t vLowCount = 0;

  if(PORTA.IN & DccSignalReceiver::mDccPinMask) {  // if DCC is high
    vNumHigh++;
    vNumLow = 0;
    vLowCount = 0;
  } else {
    vNumLow++;
    vNumHigh = 0;
    vHighCount = 0;
  }

  if(vNumHigh > NUM_CONSECUTIVE_READINGS) {
    vHighCount++;
  } else if(vNumLow > NUM_CONSECUTIVE_READINGS) {
    vLowCount++;
  }

  // check if the current half-bit is 0, 1 or invalid
  uint8_t thisHalfBit = (vHighCount < MITTELWERT) ? 1 : 0;
  
  // build the Bitstream if two consecutive half-bits are equal
  if (vLastHalfBit == 0xFF) {
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

    vLastHalfBit = 0xFF; // immer zurücksetzen nach einem Paar
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;  // clear flag
}

