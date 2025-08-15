#ifndef DCCSIGNALPARSER_H
#define DCCSIGNALPARSER_H

#include <Arduino.h>


#define DCC_MAX_BYTES   8

enum DccProtocolState{
  DCCPROTOCOL__WAIT_FOR_PREAMBLE,   // Suche nach mindestens 10 Einsen und einer Null
  DCCPROTOCOL__READ_DATA_BYTE,      // Lese 8 Datenbits
  DCCPROTOCOL__WAIT_FOR_SEPARATOR,  // Warte auf das Startbit (0) oder Stopbit (1)
  DCCPROTOCOL__PACKET_COMPLETE,     // Alle Bits eines Pakets vollständig empfangen
  DCCPROTOCOL__PACKET_VALID,        // Errorbyte wurde erfolgreich geprüft
  DCCPROTOCOL__ERROR
};

namespace DccSignalParser {
    extern char dccPacket[DCC_MAX_BYTES];
    extern uint8_t dccPacketSize;
    extern bool newDccPacket;

    void begin();
    void run();
    void addBitsToBitstream();
    void resetBitstream();
    void evaluateBitstream();
    bool findPreamble();
    uint8_t getSeparator();
    bool readDataByte();
    bool checkErrorByteOK();
    void saveDccPacket();
}
/*
class DccSignalParser {
  public:
    DccSignalParser();
    void begin(char* dccPacket, uint8_t* packetSize, bool* newDccPacket);  // z.B. PIN_PA2
    void run();

  private:
    void addBitsToBitstream();
    void resetBitstream();
    void evaluateBitstream();
    bool findPreamble();
    int8_t getSeparator();
    bool readDataByte();
    bool checkErrorByteOK();
    void saveDccPacket();

    char* mDccPacket = nullptr;
    uint8_t* mPacketSize = nullptr;
    bool* mNewDccPacket = nullptr;

    char mByteStream[DCC_MAX_BYTES];
    uint8_t mByteCount = 0;

    uint8_t mNumBits = 0;
    uint64_t mBitstream = 0;    
    uint8_t mNumOnesInPreamble = 0;

    DccProtocolState mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
};
*/

#endif