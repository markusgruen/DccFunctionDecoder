#ifndef DCCSIGNALPARSER_H
#define DCCSIGNALPARSER_H

#include <Arduino.h>


#define DCC_MAX_BYTES   8  ///< Maximum number of bytes in one DCC packet

enum DccProtocolState{
  DCCPROTOCOL__WAIT_FOR_PREAMBLE,   ///< Looking for at least 10 ones followed by a zero
  DCCPROTOCOL__READ_DATA_BYTE,      ///< Reading 8 data bits
  DCCPROTOCOL__WAIT_FOR_SEPARATOR,  ///< Waiting for start-bit (0) or stop-bit (1)
  DCCPROTOCOL__PACKET_COMPLETE,     ///< A full DCC packet has been received
  DCCPROTOCOL__ERROR                ///< Error in bitstream or framing
};

namespace DccSignalParser {
    extern bool newDccPacket;  ///< Flag: set true when a new DCC packet is available

    void begin();
    void run();
    void addBitsToBitstream();
    void resetBitstream();
    void evaluateBitstream();
    bool findPreamble();
    int8_t getSeparator();
    bool readDataByte();
    bool checkErrorByteOK();
    void saveDccPacket();
}

#endif