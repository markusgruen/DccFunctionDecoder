#ifndef DCCPACKETHANDLER_H
#define DCCPACKETHANDLER_H


#include <Arduino.h>
#include "CV_default_values.h"



enum Direction {REVERSE, FORWARD};

uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits);


class DccPacketHandler {
  public:
    DccPacketHandler(){};
    void begin(const CVDefaults* defaults, uint8_t numCVdefaults);
    void run();
    bool hasUpdate();
    Direction getDirection();
    uint8_t getSpeed();
    uint32_t getFunctions();
    void resetCVsToDefault();

  private:
    void handleDccPacket();
    int16_t getAddressFromCV();
    uint8_t getConsistAddressFromCV();
    int16_t getAddressFromDcc();
    Direction getDirectionFromDcc();
    uint8_t getSpeedFromDcc();
    uint32_t getFunctionsFromDcc();
    bool dccHasShortAddress();
    

    bool mUpdate = false;
    int16_t mAddress = -1;
    uint8_t mConsistAddress = 0;
    Direction mDirection = FORWARD;
    int8_t mSpeed = 0;
    uint32_t mFunctions = 0;

    const CVDefaults* mDefaultCVPtr = nullptr;
    uint8_t mDefaultCVcount = 0;
};




#endif