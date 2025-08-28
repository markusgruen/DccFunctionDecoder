#include "DccPacketHandler.h"
#include <EEPROM.h>
#include "CV_default_values.h"
#include "pinmap.h"


void(* resetController) (void) = 0;

namespace DccPacketHandler {
  // External variables
  uint8_t dccPacket[];
  uint8_t dccPacketSize;

  bool hasUpdate  = false;
  Direction direction = FORWARD;
  uint8_t speed = 0;
  uint32_t functions = 0UL;

  // Internal variables
  bool hasShortAddress = true;
  uint16_t address = 0xFFFF;
  uint8_t consistAddress = 0;
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
    uint8_t addressShift = (bool)dccIsLongAddress();
    uint16_t dccAddress = getAddressFromDcc();

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
        getSpeedAndDirectionFromDcc();
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
      // if(cv17 > 231) {  // remove to save flash
      //   address = 0xFFFF;
      // }
      // else {
        address = ((cv17 - 192) << 8) | cv18;
      // }
    }

    else { // short address
      address = EEPROM.read(1); // short address is stored in CV1
      // if(address < 3 || address > 127) {  // remove to save flash
      //   address = 0xFFFF;
      // }
    }
  }

  void getConsistAddressFromCV() {
    consistAddress = EEPROM.read(19) & 0b01111111;
  }

  uint16_t getAddressFromDcc() {
    if(dccIsLongAddress()) {
      return (((dccPacket[0] & 0b00111111) << 8) | dccPacket[1]);
    }
    else {
      return (dccPacket[0] & 0b01111111);
    }
  }

  void getSpeedAndDirectionFromDcc() {
    uint8_t addressShift = (bool)dccIsLongAddress();
    uint8_t oldSpeed = speed;
    Direction oldDirection = direction;
    
    
    if(dccPacket[1+addressShift] == 0x3F && dccPacketSize == 3+addressShift) { // if 127 speed steps
      speed = (dccPacket[2+addressShift] & 0b01111111) -1; // -1, because 0x01 is emergency stop and 0x02 is speed=1
      direction = bit_is_set(dccPacket[2+addressShift], 7) ? FORWARD : REVERSE;
    }
    else if(dccPacketSize == 2+addressShift) {
      speed = dccPacket[1+addressShift] & 0b00011111;
      direction = bit_is_set(dccPacket[1+addressShift], 5) ? FORWARD : REVERSE;
    }
    
    if((speed != oldSpeed) ||(direction != oldDirection)) {
      hasUpdate = true;
    }
  }

  uint32_t getFunctionsFromDcc() {
    // refactoring into the same structure as "getSpeedAndDirection" 
    // (no return, direct write of functions) DOES NOT save flash.

    uint32_t dccFunctions = functions;
    uint8_t addressShift = dccIsLongAddress() ? 1 : 0;

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

  bool dccIsLongAddress(){
    return bit_is_set(dccPacket[0], 7);
  }


  void resetCVsToDefault() {
    // for (uint8_t i = 0; i < numDefaultCVs; i++) {
    //   EEPROM.update(defaultCVs[i].address, defaultCVs[i].value);
    // }
    
    // ############## ACHTUNG #######################
    //  DER FOLGENDE CODE KOSTET EEPROM-SCHREIBZYKLEN
    //  DAFÜR SPART ER FLASH
    // 
    for (uint16_t i = 0; i < EEPROM_SIZE + 1; i++) {  // E2END = letzte EEPROM-Adresse
        eeprom_write_byte((uint8_t*)i, 0);
    }
    for(uint8_t i=0; i<numDefaultCVs; i++){
      uint16_t address = defaultCVs[i].address;
      uint8_t value    = defaultCVs[i].value;
      eeprom_write_byte((uint8_t*)address, value);
    }
  }

  void confirmCvWrite() {
    PORTA.OUTCLR = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
    for(uint16_t i=0; i<0xFFFF; i++) {
      __asm__ __volatile__("nop");
    }
    PORTA.OUTSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
  }

};

  
uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits) {
  uint32_t mask = ((1UL << numBits) - 1) << pos;
  original &= ~mask;
  return (  original |= (value << pos) & mask  );
}
