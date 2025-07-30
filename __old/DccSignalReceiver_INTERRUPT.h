#ifndef DCCSIGNALRECEIVER_H
#define DCCSGNALRECEIVER_H

#include <Arduino.h>


class DccSignalReceiver {
  public:
    DccSignalReceiver();
    void begin(int port_pin);
    void getBitstream(uint64_t* bitstream, uint8_t* numReceivedBits);


  private:
    int8_t mDccPin = 0;

};

#endif