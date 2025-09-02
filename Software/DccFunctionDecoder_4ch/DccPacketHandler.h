#ifndef DCCPACKETHANDLER_H
#define DCCPACKETHANDLER_H

#include "DccSignalParser.h"  // for "dccPacket[]" and "DCC_MAX_BYTES"
#include <Arduino.h>


enum Direction {REVERSE, FORWARD};

namespace DccPacketHandler {
  extern uint8_t dccPacket[DCC_MAX_BYTES]; ///< Raw DCC packet bytes
  extern uint8_t dccPacketSize;            ///< Number of data bytes in packet (excl. error byte)

  extern bool hasUpdate;      ///< True if new packet data is available
  extern Direction direction; ///< Current locomotive direction
  extern uint8_t speed;       ///< Current locomotive speed
  extern uint32_t functions;  ///< Current function state bits (F0..F28)

  void begin();
  void run();

  void handleDccPacket();
  void getAddressFromCV();
  void getConsistAddressFromCV();
  uint16_t getAddressFromDcc();
  Direction getDirectionFromDcc();
  uint32_t getFunctionsFromDcc();
  bool dccIsLongAddress();

  void resetCVsToDefault();
  void confirmCvWrite();
  void delay_nop();
};

/// @brief Set bits in a 32-bit integer
uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits);




#endif