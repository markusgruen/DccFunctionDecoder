#include "DccSignalReceiver_POLLING.h"
#include <Arduino.h>
#include <avr/interrupt.h>
#include "pinmap.h"


// global variables
volatile uint8_t vBitstream[RINGBUF_SIZE];
volatile uint8_t vNumReceivedBytes = 0;

// const expressions 
constexpr uint16_t US_TO_TICKS(uint16_t us) {
    return (us * (F_CPU / 1000000UL));
}
constexpr uint16_t DCC_SIGNAL_SAMPLE_TIME = US_TO_TICKS(10);
constexpr uint16_t DCC_SIGNAL_THRESHOLD = US_TO_TICKS(79);
constexpr uint16_t FILTER_SHIFT_MASK = (1<<FILTER_LENGTH);
constexpr uint8_t  FILTER_THRESHOLD = (FILTER_LENGTH+1)/2;



namespace DccSignalReceiver {
  RingBuffer ringbuf;

  /**
   * @brief Initialize DCC signal receiver (polling mode).
   *
   * Configures the DCC input pin and sets up TCA0 compare interrupt
   * to periodically sample the DCC input signal.
   */
  void begin() {
    // configure DCC pin as input 
    PORTA.DIRCLR = DCC_PIN;

    TCA0.SPLIT.CTRLD = 0;                       // ensure split mode off
    TCA0.SINGLE.CTRLB = 0;                      // normal mode
    TCA0.SINGLE.PER = 0xFFFF;                   // full 16-bit range
    TCA0.SINGLE.CMP0 = DCC_SIGNAL_SAMPLE_TIME;  // compare value
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;   // enable compare interrupt
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; // prescaler 1, enable
  }
}

/**
 * @brief Timer compare interrupt: samples the DCC input pin.
 *
 * Filters the signal, detects edges, reconstructs DCC bits and
 * pushes received bytes into the ring buffer.
 */
ISR(TCA0_CMP0_vect) {
  // This ISR takes ~3µs if no edge was detected,
  // ~5 µs if an edge was detected, and
  // ~8 µs if the received byte is saved.

  using namespace DccSignalReceiver;

  static uint8_t sFilterShiftRegister = 0;
  static uint8_t sFilterValue = 0;
  static bool sOldFilterResult = 0;

  static uint16_t sTimerCompareValue = DCC_SIGNAL_SAMPLE_TIME;
  static uint16_t sOldTimerValue = 0;
  uint16_t currentTimerValue = TCA0.SINGLE.CNT;
  static uint8_t sLastHalfBit = 0xFF;
  
  static uint8_t sBitstreamByte = 0;
  static uint8_t sBitCount = 0;
  
  
  // measure pin level
  bool isHigh = PORTA.IN & DCC_PIN;

  // moving average filter
  sFilterShiftRegister <<= 1;
  sFilterShiftRegister |= (isHigh & 0b00000001);
  sFilterValue += isHigh;
  sFilterValue -= (bool)(sFilterShiftRegister & FILTER_SHIFT_MASK);

  // edge detection
  bool thisFilterResult = sFilterValue >= FILTER_THRESHOLD;
  bool edgeDetected = thisFilterResult != sOldFilterResult;
  sOldFilterResult = thisFilterResult;

  // evaluate received bit
  if(edgeDetected) {
    uint16_t delta = (uint16_t)(currentTimerValue - sOldTimerValue);
    sOldTimerValue = currentTimerValue;
    uint8_t thisHalfBit = delta < DCC_SIGNAL_THRESHOLD;

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
          uint8_t head = ringbuf.head; // read volatile only once (to save flash)
          ringbuf.buffer[head++] = sBitstreamByte;
          ringbuf.head = (head & RINGBUF_MASK);
          sBitCount = 0;
          sBitstreamByte = 0;
        }
      }

      sLastHalfBit = 0xFF; // reset after each pair
    }
  }

  // prepare timer for next interrupt
  sTimerCompareValue += DCC_SIGNAL_SAMPLE_TIME;
  TCA0.SINGLE.CMP0 = sTimerCompareValue;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

