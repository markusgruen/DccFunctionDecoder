#include "DccSignalDecoder.h"
#include "DccSignalParser.h"
#include <EEPROM.h>

//#define bit_is_set(var, bit) ((var) & (1 << (bit)))
//#define bit_is_clear(var, bit) !bit_is_set(var, bit)


DccSignalParser parser;
char dccPacket[DCC_MAX_BYTES];
uint8_t packetSize;
bool newDccPacket;

void DccSignalDecoder::begin(int port_pin) {
  // read CVs
  mAddress = getAddressFromCV();

  parser.begin(port_pin, dccPacket, &packetSize, &newDccPacket);
}

void DccSignalDecoder::run() {
  parser.run();
  if(newDccPacket) {
    handleDccPacket();
    newDccPacket = false;
  }
}

void DccSignalDecoder::handleDccPacket() {
  if(dccPacket[0] == 0x7c) {
    EEPROM.write(dccPacket[1], dccPacket[2]); // [1] = address; [2] = value;
  }
  else if (getAddressFromDcc() == mAddress) {
    mDirection = getDirectionFromDcc();
    mFunctions = getFunctionsFromDcc();
  }

}

int16_t DccSignalDecoder::getAddressFromCV() {
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

int16_t DccSignalDecoder::getAddressFromDcc() {
  if(bit_is_clear(dccPacket[0], 7)) { // short address
    return (dccPacket[0] & 0b01111111);
  }
  else{
    return (((dccPacket[0] & 0b00111111) << 8) | dccPacket[1]);
  }
}

Direction DccSignalDecoder::getDirectionFromDcc() {
  // TODO: für einfahes DCC Format sieht das aber anders aus!!
  Direction dccDirection = bit_is_set(dccPacket[1], 7) ? FORWARD : REVERSE;
  if(dccDirection != mDirection) {
    mUpdate = true;
  }
  return dccDirection;
}

uint32_t DccSignalDecoder::getFunctionsFromDcc() {
  // TODO: check this, this comes from chatGPT
  uint32_t functions = 0;

  if (packetSize < 3) return mFunctions;

  uint8_t opcode = dccPacket[1];
  uint8_t data   = dccPacket[2];

  // F0–F4
  if ((opcode & 0b11100000) == 0b10000000) {
    if (data & (1 << 4)) functions |= (1 << 0);  // F0
    if (data & (1 << 0)) functions |= (1 << 1);  // F1
    if (data & (1 << 1)) functions |= (1 << 2);  // F2
    if (data & (1 << 2)) functions |= (1 << 3);  // F3
    if (data & (1 << 3)) functions |= (1 << 4);  // F4
  }

  // F5–F8
  else if ((opcode & 0b11111100) == 0b10110000) {
    if (data & (1 << 0)) functions |= (1 << 5);  // F5
    if (data & (1 << 1)) functions |= (1 << 6);  // F6
    if (data & (1 << 2)) functions |= (1 << 7);  // F7
    if (data & (1 << 3)) functions |= (1 << 8);  // F8
  }

  // F9–F12
  else if ((opcode & 0b11111100) == 0b10110010) {
    if (data & (1 << 0)) functions |= (1 << 9);   // F9
    if (data & (1 << 1)) functions |= (1 << 10);  // F10
    if (data & (1 << 2)) functions |= (1 << 11);  // F11
    if (data & (1 << 3)) functions |= (1 << 12);  // F12
  }

  // F13–F20
  else if ((opcode & 0b11111110) == 0b11011100) {
    functions |= (uint32_t)(data & 0xFF) << 13;  // Bits F13–F20 → Bits 13–20
  }

  // F21–F28
  else if ((opcode & 0b11111110) == 0b11011110) {
      functions |= (uint32_t)(data & 0xFF) << 21;  // Bits F21–F28 → Bits 21–28
  }

  return functions;
}

bool DccSignalDecoder::getDirection() {
  return mDirection;
}

bool DccSignalDecoder::hasUpdate() {
  bool update = mUpdate;
  mUpdate = false;
  return update;
}