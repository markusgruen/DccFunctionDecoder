#include "DccSignalReceiver_POLLING.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include "pinmap.h"


// extern volatile uint8_t DccSignalReceiver::mDccPinMask;
volatile uint8_t vTimerCompareValue;


constexpr uint16_t US_TO_TICKS(uint16_t us) {
    return (us * (F_CPU / 1000000UL));
}
constexpr uint16_t DCC_SIGNAL_SAMPLE_TIME = US_TO_TICKS(10);

constexpr uint16_t FILTER_SHIFT_MASK = (1<<FILTER_LENGTH);
constexpr uint8_t FILTER_THRESHOLD = (FILTER_LENGTH+1)/2;


#define MITTELWERT 20

volatile uint8_t vBitstream[RINGBUF_SIZE];
volatile uint8_t vNumReceivedBytes = 0;

RingBuffer ringbuf;


DccSignalReceiver::DccSignalReceiver() {
}

void DccSignalReceiver::begin() {
  // mDccPinMask = digitalPinToBitMask(pin);
  // Pin als Eingang 
  PORTA.DIRCLR = DCC_PIN;

  vTimerCompareValue = DCC_SIGNAL_SAMPLE_TIME;

  // Disable TCB before configuring
  TCB0.CTRLA = 0;

  // Use CLK_PER (20 MHz), enable, no prescaler
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc;  // Clock source: no division

  // Enable CMP interrupt, use Timebase (not periodic)
  TCB0.CTRLB = TCB_CNTMODE_INT_gc;     // Timebase mode (count up, interrupt on compare)
  TCB0.CCMP = vTimerCompareValue;  // First compare match after 200 ticks
  TCB0.INTCTRL = TCB_CAPT_bm;          // Enable compare match interrupt

  TCB0.CTRLA |= TCB_ENABLE_bm;      // Enable timer
}

bool DccSignalReceiver::getNewBitstreamByte(uint8_t* pBitstreamByte) {
  noInterrupts();
    uint8_t head = ringbuf.head;
    uint8_t tail = ringbuf.tail;
  interrupts();

  if (tail == head) {
    return false; // Buffer leer
  }

  *pBitstreamByte = ringbuf.buffer[tail++];
  
  noInterrupts();
    ringbuf.tail = (tail & RINGBUF_MASK);
  interrupts();

  return true; 
}


ISR(TCB0_INT_vect) {
  // This ISR takes ~3µs if no edge was detected,
  // ~5 µs if an edge was detected, and
  // ~8 µs if the received byte is saved.
  static uint8_t sFilterShiftRegister = 0;
  static uint8_t sFilterValue = 0;
  static bool sOldFilterResult = 0;

  static uint16_t sOldTimerValue = 0;
  uint16_t currentTimerValue = TCB0.CNT;
  static uint8_t sLastHalfBit = 0xFF;
  
  static uint8_t sBitstreamByte = 0;
  static uint8_t sBitCount = 0;
  
  
  // Pegel messen
  // bool isHigh = PORTA.IN & DccSignalReceiver::mDccPinMask;
  bool isHigh = PORTA.IN & DCC_PIN;

  // moving average of the last 5 measurements
  sFilterShiftRegister <<= 1;
  sFilterShiftRegister |= (isHigh & 0b1);
  sFilterValue += isHigh;
  sFilterValue -= (bool)(sFilterShiftRegister & FILTER_SHIFT_MASK);

  // edge detection
  bool thisFilterResult = sFilterValue >= FILTER_THRESHOLD;
  bool edgeDetected = thisFilterResult != sOldFilterResult;
  sOldFilterResult = thisFilterResult;
  
  // evaluate received bit
  if(edgeDetected) {
    uint8_t thisHalfBit = (currentTimerValue - sOldTimerValue) < THRESHOLD;
    sOldTimerValue = currentTimerValue;

    if (sLastHalfBit == 0xFF) {
      // if this is the first half-bit
      sLastHalfBit = thisHalfBit;
    }
    else {
      // if this is the second half-bit
      if (thisHalfBit == sLastHalfBit) {
        sBitstreamByte |= (thisHalfBit << sBitCount++);

        // save byte to ringbuffer if 8 bits have been received
        if(sBitCount == 8) {
          uint8_t head = ringbuf.head; // read volatile ringbuf.head only once!
          ringbuf.buffer[head++] = sBitstreamByte;
          ringbuf.head = (head & RINGBUF_MASK);
          sBitCount = 0;
        }
      }

      sLastHalfBit = 0xFF; // immer zurücksetzen nach einem Paar
    }
  }

  // prepare timer for next interrupt
  vTimerCompareValue += DCC_SIGNAL_SAMPLE_TIME;
  TCB0.CCMP = vTimerCompareValue;
  TCB0.INTFLAGS = TCB_CAPT_bm;
}



/*
ISR(TCA0_CMP0_vect) {
  // worst case: this ISR takes ~11 µs or ~220 ticks
  static uint8_t vLastHalfBit = 0xFF;
  static uint16_t vNumHigh = 0;
  static uint16_t vNumLow = 0;
  static uint16_t vHighCount = 0;
  static uint16_t vLowCount = 0;
  uint8_t thisHalfBit;

  // Pegel messen
  bool isHigh = PORTA.IN & DccSignalReceiver::mDccPinMask;

  if(isHigh) {
    vNumHigh++;
    vNumLow = 0;    
  } 
  else {
    vNumLow++;
    vNumHigh = 0;
  }
    

  if(vNumHigh > NUM_CONSECUTIVE_READINGS) {
    vHighCount++;
  } else if(vNumLow > NUM_CONSECUTIVE_READINGS) {
    vLowCount++;
  }

  bool bitReceived = false;

  if(isHigh && vLowCount > 0) {
    thisHalfBit = (vLowCount < MITTELWERT) ? 1 : 0;
    vLowCount = 0;
    bitReceived = true;
  }
  else if(!isHigh && vHighCount > 0) {
    thisHalfBit = (vHighCount < MITTELWERT) ? 1 : 0;
    vHighCount = 0;
    bitReceived = true;
  }

  // build the Bitstream if two consecutive half-bits are equal
  if(bitReceived){
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
      // else {
      //   vBitstream = 0ULL;
      //   vNumReceivedBits = 0;
      // }

      vLastHalfBit = 0xFF; // immer zurücksetzen nach einem Paar
    }
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;  // clear flag
}
*/
