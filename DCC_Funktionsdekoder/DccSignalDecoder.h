#ifndef DCCSIGNALDECODER_H
#define DCCSIGNALDECODER_H


#include <Arduino.h>

enum Direction {REVERSE, FORWARD};

class DccSignalDecoder {
  public:
    DccSignalDecoder(){};
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

    bool mUpdate = false;
    int16_t mAddress = -1;
    Direction mDirection = FORWARD;
    uint32_t mFunctions = 0;


};



#endif