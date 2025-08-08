#ifndef DCCPACKETHANDLER_H
#define DCCPACKETHANDLER_H


#include <Arduino.h>


enum Direction {REVERSE, FORWARD};

uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits);


namespace DccPacketHandler {
  extern bool hasUpdate;
  extern int16_t address;
  extern uint8_t consistAddress ;
  extern Direction direction;
  extern int8_t speed;
  extern uint32_t functions;

  void begin();
  void run();
  // bool hasUpdate();
  // Direction getDirection();
  // uint8_t getSpeed();
  // uint32_t getFunctions();
  void resetCVsToDefault();

  void handleDccPacket();
  void getAddressFromCV();
  void getConsistAddressFromCV();
  int16_t getAddressFromDcc();
  Direction getDirectionFromDcc();
  uint8_t getSpeedFromDcc();
  uint32_t getFunctionsFromDcc();
  bool dccHasShortAddress();
};




#endif