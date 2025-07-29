#ifndef DCCPACKETPARSER_H
#define DCCPACKETPARSER_H


#include <Arduino.h>

enum Direction {REVERSE, FORWARD};

class DccPacketParser {
  public:
    DccPacketParser(){};
    void begin(int port_pin);
    void run();
    bool hasUpdate();
    bool getDirection();
    uint32_t getFunctions();

  private:
    void handleDccPacket();
    int16_t getAddressFromCV();
    int16_t getAddressFromDcc();
    Direction getDirectionFromDcc();
    uint32_t getFunctionsFromDcc();
    bool hasShortAddress();

    bool mUpdate = false;
    int16_t mAddress = -1;
    Direction mDirection = FORWARD;
    uint32_t mFunctions = 0;


};

uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits);


#endif