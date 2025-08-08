#include "DccPacketHandler.h"
#include "DccSignalParser.h"
#include <EEPROM.h>
#include "CV_default_values.h"


// char dccPacket[DCC_MAX_BYTES];
// uint8_t dccPacketSize;
// bool newDccPacket;


void(* resetController) (void) = 0;

namespace DccPacketHandler {
  bool hasUpdate  = false;
  int16_t address =- 1;
  uint8_t consistAddress = 0;
  Direction direction = FORWARD;
  int8_t speed = 0;
  uint32_t functions = 0UL;



  void begin() {
    // read CVs
    getAddressFromCV();
    getConsistAddressFromCV();

    DccSignalParser::begin();
  }

  void run() {
    DccSignalParser::run();
    if(DccSignalParser::newDccPacket) {
      handleDccPacket();
      DccSignalParser::newDccPacket = false;
    }
  }

  // Direction DccPacketHandler::getDirection() {
  //   mUpdate = false;
  //   return mDirection;
  // }

  // uint8_t DccPacketHandler::getSpeed() {
  //   mUpdate = false;
  //   return mSpeed;
  // }

  // uint32_t DccPacketHandler::getFunctions(){
  //   mUpdate = false;
  //   return mFunctions;
  // }

  void handleDccPacket() {
    uint16_t dccAddress = getAddressFromDcc();
    
    // write CV
    if(DccSignalParser::dccPacket[0] == 0x7c) {
      if(DccSignalParser::dccPacket[1] == 7 && DccSignalParser::dccPacket[2] == 8) { // CV8 = 8 means reset
        resetCVsToDefault();
        resetController();
      }
      else {
        EEPROM.update(DccSignalParser::dccPacket[1], DccSignalParser::dccPacket[2]); // [1] = address; [2] = value;
      }
    }
    // standard packet
    else if (dccAddress == address || dccAddress == consistAddress) {
      direction = getDirectionFromDcc();
      speed = getSpeedFromDcc();
      functions = getFunctionsFromDcc();
    }
  }

  void getAddressFromCV() {
    uint8_t cv29 = EEPROM.read(29);

    // check if short or long address:
    if(bit_is_set(cv29, 4)) {
      uint8_t cv17 = EEPROM.read(17);
      uint8_t cv18 = EEPROM.read(18);
      if(cv17 > 231) {
        address = -1;
      }
      else {
        address = ((cv17 - 192) << 8) | cv18;
      }
    }

    else { // short address
      address = EEPROM.read(1); // short address is stored in CV1
      if(address < 3 || address > 127) {
        address = -1;
      }
    }
  }

  void getConsistAddressFromCV() {
    consistAddress = EEPROM.read(19) & 0b01111111;
  }

  int16_t getAddressFromDcc() {
    if(dccHasShortAddress()) {
      return (DccSignalParser::dccPacket[0] & 0b01111111);
    }
    else{
      return (((DccSignalParser::dccPacket[0] & 0b00111111) << 8) | DccSignalParser::dccPacket[1]);
    }
  }

  uint8_t getSpeedFromDcc() {
    uint8_t dccSpeed = 0;
    uint8_t addressShift = dccHasShortAddress() ? 0 : 1;
    
    if(DccSignalParser::dccPacket[1] == 0x3F && DccSignalParser::dccPacketSize == 3+addressShift) {
      dccSpeed = DccSignalParser::dccPacket[2+addressShift] & 0b01111111;
    }
    else if(DccSignalParser::dccPacketSize == 2+addressShift) {
      dccSpeed = DccSignalParser::dccPacket[1+addressShift] & 0b00011111;
    }
    
    if(dccSpeed != speed) {
      hasUpdate = true;
    }
    return dccSpeed;
  }

  Direction getDirectionFromDcc() {
    Direction dccDirection = FORWARD;
    uint8_t addressShift = dccHasShortAddress() ? 0 : 1;

    
    if(DccSignalParser::dccPacket[1] == 0x3F && DccSignalParser::dccPacketSize == 3+addressShift) {
      dccDirection = bit_is_set(DccSignalParser::dccPacket[2+addressShift], 7) ? FORWARD : REVERSE;
    }
    else if(DccSignalParser::dccPacketSize == 2+addressShift) {
      dccDirection = bit_is_set(DccSignalParser::dccPacket[1+addressShift], 5) ? FORWARD : REVERSE;
    }

    if(dccDirection != direction) {
      hasUpdate = true;
    }
    return dccDirection;
  }

  uint32_t getFunctionsFromDcc() {
    uint32_t dccFunctions = 0;
    uint8_t addressShift = dccHasShortAddress() ? 0 : 1;

    switch (DccSignalParser::dccPacket[1+addressShift] & 0b11100000) {
      case 0b10000000:
        dccFunctions = setBitsUint32(dccFunctions, 0, (DccSignalParser::dccPacket[1+addressShift]>>4), 1);
        dccFunctions = setBitsUint32(dccFunctions, 1, DccSignalParser::dccPacket[1+addressShift], 4);
        break;
      case 0b10110000:
        dccFunctions = setBitsUint32(dccFunctions, 5, DccSignalParser::dccPacket[1+addressShift], 4);
        break;
      case 0b10100000:
        dccFunctions = setBitsUint32(dccFunctions, 9, DccSignalParser::dccPacket[1+addressShift], 4);
        break;
      case 0xDE:
        if(DccSignalParser::dccPacketSize >= 3){
          dccFunctions = setBitsUint32(dccFunctions, 13, DccSignalParser::dccPacket[2+addressShift], 8);
        }
        break;
      case 0xDF:
        if(DccSignalParser::dccPacketSize >= 3){
          dccFunctions = setBitsUint32(dccFunctions, 21, DccSignalParser::dccPacket[2+addressShift], 8);
        }
        break;  
    }
    
    if(dccFunctions != functions) {
      hasUpdate = true;
    }
    return dccFunctions;
  }

  // bool DccPacketHandler::hasUpdate() {
  //   bool hasUpdate = mUpdate;
  //   mUpdate = false;
  //   return update;
  // }

  bool dccHasShortAddress(){
    return bit_is_set(DccSignalParser::dccPacket[0], 7);
  }

  void resetCVsToDefault() {
    for (uint8_t i = 0; i < numDefaultCVs; i++) {
      EEPROM.update(defaultCVs[i].address, defaultCVs[i].value);
    }
  }

  uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits) {
    uint32_t mask = ((1UL << numBits) - 1) << pos;
    original &= ~mask;
    return (  original |= (value << pos) & mask  );
  }
};