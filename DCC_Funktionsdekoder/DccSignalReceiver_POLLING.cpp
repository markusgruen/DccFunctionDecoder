#include "DccSignalReceiver_POLLING.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include "pinmap.h"


// global variables
volatile uint8_t vBitstream[RINGBUF_SIZE];
volatile uint8_t vNumReceivedBytes = 0;
volatile uint8_t vTimerCompareValue;
//RingBuffer ringbuf;

// const expressions and #defines
#define MITTELWERT 20  // <-- change to constexpr
constexpr uint16_t US_TO_TICKS(uint16_t us) {
    return (us * (F_CPU / 1000000UL));
}
constexpr uint16_t DCC_SIGNAL_SAMPLE_TIME = US_TO_TICKS(10);
constexpr uint16_t FILTER_SHIFT_MASK = (1<<FILTER_LENGTH);
constexpr uint8_t FILTER_THRESHOLD = (FILTER_LENGTH+1)/2;




namespace DccSignalReceiver {
  RingBuffer ringbuf;

  void begin() {
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

/*
  // TODO: den Rest umbauen, dann braucht man das nicht mehr
  bool getNewBitstreamByte(uint8_t* pBitstreamByte) {
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
*/
}

ISR(TCB0_INT_vect) {
  using namespace DccSignalReceiver;

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

