#ifndef DCCDECODER_H
#define DCCDECODER_H

#include <stdint.h>
#include <avr/io.h>

class DCCDecoder {
public:
    DCCDecoder();
    void begin(uint8_t port_pin);  // z.B. PIN_PA2
    void run();

private:
    void findPreamble();
    void findStartBit();
    void readAddressByte();
    
    void resetBitStream();
    int8_t mLastBit = -1;
    uint8_t mValidBitCount = 0;
    uint16_t mBitstream
    DccProtocolState mState = DccProtocolState::WAIT_FOR_PREAMBLE;

};

#endif
