#include "DccPacketParser.h"
#include "DccSignalParser.h"
#include <EEPROM.h>

//#define bit_is_set(var, bit) ((var) & (1 << (bit)))
//#define bit_is_clear(var, bit) !bit_is_set(var, bit)


DccSignalParser parser;
char dccPacket[DCC_MAX_BYTES];
uint8_t dccPacketSize;
bool newDccPacket;

void DccPacketParser::begin(int port_pin) {
  // read CVs
  mAddress = getAddressFromCV();

  parser.begin(port_pin, dccPacket, &dccPacketSize, &newDccPacket);
}

void DccPacketParser::run() {
  parser.run();
  if(newDccPacket) {
    handleDccPacket();
    newDccPacket = false;
  }
}

bool DccPacketParser::getDirection() {
  return mDirection;
}

uint32_t DccPacketParser::getFunctions(){
  return mFunctions;
}

void DccPacketParser::handleDccPacket() {
  if(dccPacket[0] == 0x7c) {
    EEPROM.write(dccPacket[1], dccPacket[2]); // [1] = address; [2] = value;
  }
  else if (getAddressFromDcc() == mAddress) {
    mDirection = getDirectionFromDcc();
    mFunctions = getFunctionsFromDcc();
  }

}

int16_t DccPacketParser::getAddressFromCV() {
  int16_t address = -1;
  uint8_t cv29 = EEPROM.read(29);

  // check if short or long address:
  if(bit_is_set(cv29, 5)) {
    uint8_t cv17 = EEPROM.read(17);
    uint8_t cv18 = EEPROM.read(18);
    if(cv17 < 192 || cv17 > 231) {
      address = -1;
    }
    else {
      address = ((cv17 - 192) << 8) | cv18;
    }

  }

  else { // short address
    address = EEPROM.read(1); // short address is stored in CV1
    if(mAddress < 3 || mAddress > 127) {
      address = -1;
    }
  }
  return address;
}

int16_t DccPacketParser::getAddressFromDcc() {
  if(hasShortAddress()) {
    return (dccPacket[0] & 0b01111111);
  }
  else{
    return (((dccPacket[0] & 0b00111111) << 8) | dccPacket[1]);
  }
}

Direction DccPacketParser::getDirectionFromDcc() {
  Direction dccDirection = FORWARD;

  if(dccPacketSize == 2) {
    dccDirection = bit_is_set(dccPacket[1], 5) ? FORWARD : REVERSE;
  }
  else if(hasShortAddress()) {
    dccDirection = bit_is_set(dccPacket[2], 7) ? FORWARD : REVERSE;
  }
  else if(dccPacket[2] == 0x3F) {
    dccDirection = bit_is_set(dccPacket[3], 7) ? FORWARD : REVERSE;
  }
  else {
    dccDirection = bit_is_set(dccPacket[2], 5) ? FORWARD : REVERSE;
  }

  if(dccDirection != mDirection) {
    mUpdate = true;
  }
  return dccDirection;
}

uint32_t DccPacketParser::getFunctionsFromDcc() {
  // TODO: check this, this comes from chatGPT
  uint32_t functions = 0;
  uint8_t addressShift = 1;

  if(hasShortAddress()) {
    addressShift = 0;
  }

  switch (dccPacket[1+addressShift] & 0b11100000) {
    case 0b10000000:
      functions = setBitsUint32(functions, 0, (dccPacket[1+addressShift]>>4), 1);
      functions = setBitsUint32(functions, 1, dccPacket[1+addressShift], 4);
      break;
    case 0b10110000:
      functions = setBitsUint32(functions, 5, dccPacket[1+addressShift], 4);
      break;
    case 0b10100000:
      functions = setBitsUint32(functions, 9, dccPacket[1+addressShift], 4);
      break;
    case 0xDE:
      if(dccPacketSize >= 3){
        functions = setBitsUint32(functions, 13, dccPacket[2+addressShift], 8);
      }
      break;
    case 0xDF:
      if(dccPacketSize >= 3){
        functions = setBitsUint32(functions, 21, dccPacket[2+addressShift], 8);
      }
      break;  
  }
  
  return functions;
}

bool DccPacketParser::hasUpdate() {
  bool update = mUpdate;
  mUpdate = false;
  return update;
}

bool DccPacketParser::hasShortAddress(){
  return bit_is_set(dccPacket[0], 7);
}

uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits) {
  uint32_t mask = ((1UL << numBits) - 1) << pos;
  original &= ~mask;
  return (  original |= (value << pos) & mask  );
}