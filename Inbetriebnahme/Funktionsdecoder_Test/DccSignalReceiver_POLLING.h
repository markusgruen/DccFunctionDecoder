#ifndef DCCSIGNALRECEIVER_POLLING_H
#define DCCSIGNALRECEIVER_POLLING_H

#include <Arduino.h>


#define FILTER_LENGTH 3   // simple input filter length

#define RINGBUF_SIZE 8    // must be power of two
#define RINGBUF_MASK (RINGBUF_SIZE - 1) // for fast modulo operation

// Ring buffer structure for ISR ↔ main loop communication
typedef struct {
  uint8_t buffer[RINGBUF_SIZE];     // data buffer (non-volatile !!! )
  volatile uint8_t head;            // write index (set by ISR)
  volatile uint8_t tail;            // read index (used by DccSignalParser)
} RingBuffer;


namespace DccSignalReceiver {
    extern RingBuffer ringbuf;      // shared receive buffer

    void begin();                   // initialize receiver
}


#endif