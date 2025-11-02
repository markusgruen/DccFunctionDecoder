#include "DccPacketHandler.h"
#include <EEPROM.h>
#include "CV_default_values.h"
#include "CV_table.h"
#include "pinmap.h"
#include "outputController.h" // for "readCVs" und "dimmValue"

/// Function pointer for software reset
void(* resetController) (void) = 0;

namespace DccPacketHandler {
  // External variables
  uint8_t dccPacket[];
  uint8_t dccPacketSize;

  Direction direction = FORWARD; ///< Current direction
  uint32_t functions = 0UL;      ///< Current function bitmask

  // Internal variables
  uint16_t address = DEFAULT_SHORT_ADDRESS;
  uint16_t magicAddress = MAGIC_ADDRESS;
  uint8_t consistAddress = 0;
  uint8_t lastDccErrorByte = 0;

  /**
   * @brief Initialize the DCC packet handler (read CVs)
   */
  void begin() {
    // read CVs
    getAddressFromCV();
    getConsistAddressFromCV();

    DccSignalParser::begin();
  }

  /**
   * @brief Main loop, processes new packets when available
   */
  void run() {
    DccSignalParser::run();
    if(DccSignalParser::newDccPacket) {
      handleDccPacket();
      DccSignalParser::newDccPacket = false;
    }
  }

  /**
   * @brief Decode and act on the current DCC packet
   * @details Handles CV writes, direction, and function updates
   */
  void handleDccPacket() {
    uint8_t addressShift = (bool)dccIsLongAddress();
    uint16_t dccAddress = getAddressFromDcc();

    if(dccAddress > 0) {
      // --- Handle CV writes ---
      if((dccAddress == address || dccAddress == magicAddress) && ((dccPacket[1+addressShift] & 0b11111100) == 0b11101100)) {
        if(dccPacket[dccPacketSize-1] == lastDccErrorByte) {  // confirm that two consecutive CV-write commands have been received 
          if(dccPacket[2+addressShift] == 7 && dccPacket[3+addressShift] == 8 ) { // Decoder reset?
            // Decoder reset
            resetCVsToDefault();

            for(uint8_t i=0; i<10; i++) {
              confirmCvWrite();
            }

            resetController();
          }
          else {
            EEPROM.update(dccPacket[2+addressShift]+1, dccPacket[3+addressShift]); // [1] = address; [2] = value;
            confirmCvWrite();
            if(dccPacket[2+addressShift]+1 == CONFIGBYTE) {  // update address if CV 29 was written
              getAddressFromCV();           
            }
            else {
              getConsistAddressFromCV();
              OutputController::readCVs();
              vPwmValue[0] = OutputController::dimmValue[0];
              vPwmValue[1] = OutputController::dimmValue[1];
              vPwmValue[2] = OutputController::dimmValue[2];
              vPwmValue[3] = OutputController::dimmValue[3];
            }
          }   
        }
        lastDccErrorByte = dccPacket[dccPacketSize-1];
        return;
      }
      
      // --- Handle speed & function commands ---
      if(dccAddress == address || dccAddress == consistAddress) {
        if(bit_is_clear(dccPacket[1+addressShift], 7)) { // is speed packet
          direction = getDirectionFromDcc();
        }
        else if(bit_is_set(dccPacket[1+addressShift], 7)) {  // is function packet
          functions = getFunctionsFromDcc();
        }
      }
    }

    lastDccErrorByte = dccPacket[dccPacketSize-1];
  }

  /**
   * @brief Load locomotive address from EEPROM CVs
   */
  void getAddressFromCV() {
    uint8_t cv29 = EEPROM.read(CONFIGBYTE);

    if(bit_is_set(cv29, 5)) {  // if long address
      uint8_t cv17 = EEPROM.read(LONGADDRESS1);
      uint8_t cv18 = EEPROM.read(LONGADDRESS2);
      address = ((cv17 - 192) << 8) | cv18;
      if(address <= 127 || address > 10239) {
        address = DEFAULT_LONG_ADDRESS;
      }
    }

    else { // short address
      address = EEPROM.read(SHORTADDRESS);
      if(address == 0 || address > 127) {
        address = DEFAULT_SHORT_ADDRESS;
      }
    }
  }

  /**
   * @brief Load consist address from EEPROM
   */
  void getConsistAddressFromCV() {
    consistAddress = EEPROM.read(CONSISTADDRESS) & 0b01111111;
  }

  /**
   * @brief Extract address from current DCC packet
   * @return 7-bit (short) or 14-bit (long) address
   */
  uint16_t getAddressFromDcc() {
    if(dccIsLongAddress()) {
      return (((dccPacket[0] & 0b00111111) << 8) | dccPacket[1]);
    }
    else {
      return (dccPacket[0] & 0b01111111);
    }
  }

  /**
   * @brief Extract direction from DCC speed packet
   * @return FORWARD or REVERSE
   */
  Direction getDirectionFromDcc() {
    uint8_t addressShift = (bool)dccIsLongAddress();
    
    if(dccPacket[1+addressShift] == 0x3F && dccPacketSize == 3+addressShift) { // if 127 speed steps
      direction = bit_is_set(dccPacket[2+addressShift], 7) ? FORWARD : REVERSE;
    }
    else if(dccPacketSize == 2+addressShift) {
      direction = bit_is_set(dccPacket[1+addressShift], 5) ? FORWARD : REVERSE;
    }
    
    return direction;
  }

  /**
   * @brief Extract functions from DCC function packet
   * @return Updated 32-bit function mask (F0..F20)
   */
  uint32_t getFunctionsFromDcc() {
    // refactoring into the same structure as "getSpeedAndDirection" 
    // (no return, direct write of functions) DOES NOT save flash.

    uint32_t dccFunctions = functions;
    uint8_t addressShift = (bool)dccIsLongAddress();

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
      if(dccPacketSize >= 3){
          dccFunctions = setBitsUint32(dccFunctions, dccPacket[2+addressShift], 13, 8);
      }
    }
    // With the current setup, the function map CV's last two bits (bit 23 and bit 22)
    // define forward only / reverse only. Therefore, the Functions F21...28 are not supported
    // else if(dccPacket[1+addressShift] == (uint8_t)0b11011111){
    //   if(dccPacketSize >= 3){
    //       dccFunctions = setBitsUint32(dccFunctions, dccPacket[2+addressShift], 21, 8);
    //   }
    // }

    return dccFunctions;
  }

  /**
   * @brief Check if current packet uses a long address
   * @return true if long address, false otherwise
   */
  bool dccIsLongAddress(){
    return bit_is_set(dccPacket[0], 7);
  }

  /**
   * @brief Reset all CVs to default values
   */
  void resetCVsToDefault() {
    for (uint16_t i = 0; i < EEPROM_SIZE + 1; i++) {
        eeprom_write_byte((uint8_t*)i, 0);
    }
    for(uint8_t i=0; i<numDefaultCVs; i++){
      uint16_t address = defaultCVs[i].address;
      uint8_t value    = defaultCVs[i].value;
      eeprom_write_byte((uint8_t*)address, value);
    }
  }

  /**
   * @brief Flashes all AUX-outputs to confirm CV write
   */
  void confirmCvWrite() {
    constexpr uint8_t allPins = CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN;

    TCB0.INTCTRL = 0;        // Interrupt disable

    PORTA.OUTSET = allPins;
    delay_nop();
    PORTA.OUTCLR = allPins;
    delay_nop();
    PORTA.OUTSET = allPins;
    delay_nop();

    TCB0.INTCTRL = TCB_CAPT_bm;        // Interrupt enable
  }

  /**
   * @brief Small delay using NOP instructions
   */
  void delay_nop() {
    for(uint16_t i=0; i<0xFFFF; i++) {
      __asm__ __volatile__("nop");
    }
  }
};

/**
 * @brief Set bits in a 32-bit integer
 * @param original Original value
 * @param value Value to insert
 * @param pos Starting bit position
 * @param numBits Number of bits to overwrite
 * @return Modified 32-bit integer
 */
uint32_t setBitsUint32(uint32_t original, uint32_t value, uint8_t pos, uint8_t numBits) {
  uint32_t mask = ((1UL << numBits) - 1) << pos;
  original &= ~mask;
  return (  original |= (value << pos) & mask  );
}
