#include "DccPacketHandler.h"
#include <EEPROM.h>
#include "CV_default_values.h"
#include "pinmap.h"


void(* resetController) (void) = 0;

namespace DccPacketHandler {
  uint8_t dccPacket[];
  uint8_t dccPacketSize;

  bool hasUpdate  = false;
  uint16_t address = 0xFFFF;
  uint8_t consistAddress = 0;
  Direction direction = FORWARD;
  uint8_t speed = 0;
  uint32_t functions = 0UL;


  uint8_t lastDccErrorByte = 0;


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

  void handleDccPacket() {
    uint16_t dccAddress = getAddressFromDcc();
    uint8_t addressShift = dccIsShortAddress() ? 0 : 1;

    // only write CVs when this is the "real" address
    if((dccAddress == address)) {

      // write CVs 
      if((dccPacket[1+addressShift] & 0b11111100) == 0b11101100){  // if "write Byte to CV"
        if(dccPacket[dccPacketSize-1] == lastDccErrorByte) {  // confirm that two consecutive CV-write commands have been received 
          if(dccPacket[2+addressShift] == 7 && dccPacket[3+addressShift] == 8 ) { // Decoder reset?
            resetCVsToDefault();

            for(uint8_t i=0; i<50; i++) {
              confirmCvWrite();
            }

            resetController();
          }
          else {
            EEPROM.update(dccPacket[2+addressShift]+1, dccPacket[3+addressShift]); // [1] = address; [2] = value;
            confirmCvWrite();
            if(dccPacket[2+addressShift]+1 == 29) {  // update address if CV 29 was written
              getAddressFromCV();           
            }
          }   
        }

        lastDccErrorByte = dccPacket[dccPacketSize-1];
        return;
      }
    }
    
    if(dccAddress == address || dccAddress == consistAddress) {
      if(bit_is_clear(dccPacket[1+addressShift], 7)) { // is speed packet
        direction = getDirectionFromDcc();
        speed = getSpeedFromDcc();
      }
      else if(bit_is_set(dccPacket[1+addressShift], 7)) {  // is function packet
        functions = getFunctionsFromDcc();
      }
    }

    lastDccErrorByte = dccPacket[dccPacketSize-1];
  }

  void getAddressFromCV() {
    uint8_t cv29 = EEPROM.read(29);

    // check if short or long address:
    if(bit_is_set(cv29, 4)) {
      uint8_t cv17 = EEPROM.read(17);
      uint8_t cv18 = EEPROM.read(18);
      if(cv17 > 231) {
        address = 0xFFFF;
      }
      else {
        address = ((cv17 - 192) << 8) | cv18;
      }
    }

    else { // short address
      address = EEPROM.read(1); // short address is stored in CV1
      if(address < 3 || address > 127) {
        address = 0xFFFF;
      }
    }
  }

  void getConsistAddressFromCV() {
    consistAddress = EEPROM.read(19) & 0b01111111;
  }

  int16_t getAddressFromDcc() {
    if(dccIsShortAddress()) {
      return (dccPacket[0] & 0b01111111);
    }
    else{
      return (((dccPacket[0] & 0b00111111) << 8) | dccPacket[1]);
    }
  }

  uint8_t getSpeedFromDcc() {
    uint8_t dccSpeed = 0;
    uint8_t addressShift = dccIsShortAddress() ? 0 : 1;
    
    if(dccPacket[1] == 0x3F && dccPacketSize == 3+addressShift) { // if 127 speed steps
      dccSpeed = (dccPacket[2+addressShift] & 0b01111111) -1; // -1, because 0x01 is emergency stop and 0x02 is speed=1
    }
    else if(dccPacketSize == 2+addressShift) {
      dccSpeed = dccPacket[1+addressShift] & 0b00011111;
    }
    
    if(dccSpeed != speed) {
      hasUpdate = true;
    }

    return dccSpeed;
  }

  Direction getDirectionFromDcc() {
    Direction dccDirection = FORWARD;
    uint8_t addressShift = dccIsShortAddress() ? 0 : 1;

    
    if(dccPacket[1] == 0x3F && dccPacketSize == 3+addressShift) {
      dccDirection = bit_is_set(dccPacket[2+addressShift], 7) ? FORWARD : REVERSE;
    }
    else if(dccPacketSize == 2+addressShift) {
      dccDirection = bit_is_set(dccPacket[1+addressShift], 5) ? FORWARD : REVERSE;
    }

    if(dccDirection != direction) {
      hasUpdate = true;
    }

    return dccDirection;
  }

  uint32_t getFunctionsFromDcc() {
    uint32_t dccFunctions = functions;
    uint8_t addressShift = dccIsShortAddress() ? 0 : 1;

    if((dccPacket[1+addressShift] & 0b11100000) == (uint8_t)0b10000000) {
      dccFunctions = setBitsUint32(dccFunctions, (dccPacket[1+addressShift]>>4), 0, 1);
      dccFunctions = setBitsUint32(dccFunctions, dccPacket[1+addressShift], 1, 4);
    }
    else if((dccPacket[1+addressShift] & 0b11110000) == (uint8_t)0b10110000){
      dccFunctions = setBitsUint32(dccFunctions, dccPacket[1+addressShift], 5, 4);
    }
    else if((dccPacket[1+addressShift] & 0b11110000) == (uint8_t)0b10100000){
      dccFunctions = setBitsUint32(dccFunctions, dccPacket[1+addressShift], 9, 4);
    }
    else if(dccPacket[1+addressShift] == (uint8_t)0b11011110) {
      if(dccPacketSize >= 3){  // make sure to not read the checksum <-- is this necessary?
          dccFunctions = setBitsUint32(dccFunctions, dccPacket[2+addressShift], 13, 8);
      }
    }
    else if(dccPacket[1+addressShift] == (uint8_t)0b11011111){
      if(dccPacketSize >= 3){ // make sure to not read the checksum <-- is this necessary?
          dccFunctions = setBitsUint32(dccFunctions, dccPacket[2+addressShift], 21, 8);
      }
    }
   
    if(dccFunctions != functions) {
      hasUpdate = true;
    }

    return dccFunctions;
  }

  inline bool dccIsShortAddress(){
    return bit_is_clear(dccPacket[0], 7);
  }


  void resetCVsToDefault() {
    for (uint8_t i = 0; i < numDefaultCVs; i++) {
      EEPROM.update(defaultCVs[i].address, defaultCVs[i].value);
    }
  }

  inline void confirmCvWrite() {
    PORTA.OUTCLR = CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN;
    for(uint16_t i=0; i<0xFFFF; i++) {
      __asm__ __volatile__("nop");
    }
    PORTA.OUTSET = CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN;
  }

};

  
uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits) {
  uint32_t mask = ((1UL << numBits) - 1) << pos;
  original &= ~mask;
  return (  original |= (value << pos) & mask  );
}
