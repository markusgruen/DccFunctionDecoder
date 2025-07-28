#ifndef DCCDECODER_H
#define DCCDECODER_H

#include <stdint.h>
//#include <avr/io.h>

#define DCC_MAX_BYTES   8

enum class DccProtocolState : uint8_t {
  WAIT_FOR_PREAMBLE,   // Suche nach mindestens 10 Einsen
  WAIT_FOR_SEPARATOR,  // Warte auf das Startbit (0) oder Stopbit (1)
  READ_DATA_BYTE,      // Lese 8 Datenbits
  PACKET_COMPLETE,     // Paket vollständig empfangen
  ERROR                // Ungültiger Zustand oder Fehler
};

class DCCDecoder {
public:
    DCCDecoder();
    void begin(uint8_t port_pin);  // z.B. PIN_PA2
    void run();
    uint8_t getBytestream(char *bytestream);

    void TESTsetBitstream(uint64_t bitstream, uint8_t numValidBits);


private:
    void addBitToBitstream(int8_t bit);
    void resetBitstream();
    void evaluateBitstream();
    bool findPreamble();
    int8_t getSeparator();
    bool readDataByte();
    
    int8_t mLastBit = -1;
    uint8_t mValidBitCount = 0;
    uint64_t mBitstream = 0;
    char mBytestream[DCC_MAX_BYTES];
    uint8_t mByteCount = 0;
    uint8_t mNumOnesInPreamble = 0;
    DccProtocolState mState = DccProtocolState::WAIT_FOR_PREAMBLE;

};

#endif
