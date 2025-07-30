#ifndef DCCSIGNALRECEIVER_POLLING_H
#define DCCSGNALRECEIVER_POLLING_H

#include <Arduino.h>


#define NUM_CONSECUTIVE_READINGS 5


class DccSignalReceiver {
  public:
    DccSignalReceiver();
    static void begin(uint8_t pinMask);
    void getBitstream(uint64_t* bitstream, uint8_t* numReceivedBits);

    static volatile uint8_t mDccPinMask;
};

#endif