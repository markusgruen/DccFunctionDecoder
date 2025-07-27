#ifndef DCCSIGNALDECODER_H
#define DCCSIGNALDECODER_H

#include <Arduino.h>


#define DCC_MAX_BYTES   8

enum DccProtocolState{
  DCCPROTOCOL__WAIT_FOR_PREAMBLE,   // Suche nach mindestens 10 Einsen und einer Null
  DCCPROTOCOL__READ_DATA_BYTE,      // Lese 8 Datenbits
  DCCPROTOCOL__WAIT_FOR_SEPARATOR,  // Warte auf das Startbit (0) oder Stopbit (1)
  DCCPROTOCOL__PACKET_COMPLETE,     // Paket vollständig empfangen
  DCCPROTOCOL__ERROR                // Ungültiger Zustand oder Fehler
};

class DccSignalDecoder {
public:
    DccSignalDecoder();
    void begin(uint8_t port_pin);  // z.B. PIN_PA2
    void run();
    uint8_t getBytestream(char *bytestream);

    void TESTsetBitstream(uint64_t bitstream, uint8_t numValidBits);

private:
  void addBitsToBitstream();
    //void addBitToBitstream(int8_t bit);
    void resetBitstream();
    void evaluateBitstream();
    bool findPreamble();
    int8_t getSeparator();
    bool readDataByte();

    uint8_t mNumBits = 0;
    uint64_t mBitstream = 0;    
    uint8_t mNumOnesInPreamble = 0;

    char mBytestream[DCC_MAX_BYTES];
    uint8_t mByteCount = 0;
    
    DccProtocolState mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
};






#endif