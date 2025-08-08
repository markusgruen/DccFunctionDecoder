#ifndef DCCSIGNALRECEIVER_POLLING_H
#define DCCSIGNALRECEIVER_POLLING_H

#include <Arduino.h>


//#define NUM_CONSECUTIVE_READINGS 5
#define THRESHOLD 100
#define FILTER_LENGTH 5

#define RINGBUF_SIZE 8
#define RINGBUF_MASK (RINGBUF_SIZE - 1)

typedef struct {
  uint8_t buffer[RINGBUF_SIZE];     // NICHT volatile
  volatile uint8_t head;            // wird von ISR geschrieben
  volatile uint8_t tail;            // wird vom Hauptprogramm gelesen
} RingBuffer;


namespace DccSignalReceiver {
    extern RingBuffer ringbuf;

    void begin();
    //bool getNewBitstreamByte(uint8_t* bitstreamByte);
}

/*
class DccSignalReceiver {
  public:
    DccSignalReceiver();
    static void begin();
    bool getNewBitstreamByte(uint8_t* bitstreamByte);

    // static volatile uint8_t mDccPinMask;
};
*/

#endif