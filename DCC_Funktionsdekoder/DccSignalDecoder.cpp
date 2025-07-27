#include "DccSignalDecoder.h"
#include "DccSignalReceiver.h"

#include <cstring>
#include <bitset>
#include <iostream>


DccSignalReceiver receiver;


DccSignalDecoder::DccSignalDecoder() {
}

void DccSignalDecoder::begin(uint8_t pin){
  receiver.begin(pin);
}

void DccSignalDecoder::addBitsToBitstream() {
  uint8_t numNewBits = 0;
  uint64_t newBitstream = 0;
  
  receiver.getBitstream(&newBitstream, &numNewBits);
  mBitstream |= (newBitstream << mNumBits);
  mNumBits += numNewBits;
}


void DccSignalDecoder::resetBitstream(){
  mBitstream = 0;
  mNumBits = 0;
  mByteCount = 0;
  mNumOnesInPreamble = 0;
  mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
}

void DccSignalDecoder::evaluateBitstream() {
  int separator = -1;
  
  switch(mState) {
    case DCCPROTOCOL__WAIT_FOR_PREAMBLE:
      if(findPreamble() == true){
        mState = DCCPROTOCOL__READ_DATA_BYTE;
      }
      break;

    case DCCPROTOCOL__READ_DATA_BYTE:
      if(readDataByte() == true) {
        mState = DCCPROTOCOL__WAIT_FOR_SEPARATOR;
      }
      break;

    case DCCPROTOCOL__WAIT_FOR_SEPARATOR:
      separator = getSeparator();
      if(separator == 0) {
        mState = DCCPROTOCOL__READ_DATA_BYTE;
      } else if(separator == 1) {
        mState = DCCPROTOCOL__PACKET_COMPLETE;
      } else { // no separator received yet, do nothing
      }
      break;

    case DCCPROTOCOL__PACKET_COMPLETE:
      // TODO: evaluate checksum
      break;

    case DCCPROTOCOL__ERROR:
      resetBitstream();
      mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
      break;
  }
}

bool DccSignalDecoder::findPreamble() {
  bool bit = 0;
  
  while(mNumBits > 0) {
    bit =(mBitstream & 0b1);
    mBitstream >>= 1;
    mNumBits--;

    if(bit == 1) {
      mNumOnesInPreamble++;
    }
    else {
      if(mNumOnesInPreamble >= 10) {
        // zero was found and >= 10 ones before this zero
        mNumOnesInPreamble = 0;
        return true;
      }
      else {
        // zero was found but not enough ones
        mNumOnesInPreamble = 0;
      }
    }
  }
  return false;
}

int8_t DccSignalDecoder::getSeparator() {
    
  int8_t separator = -1;
  if(mNumBits >= 1) {
    separator = (mBitstream & 0b1);
    mBitstream >>= 1;
    mNumBits--;
  }
  return separator;
}

bool DccSignalDecoder::readDataByte() {
  if(mNumBits >= 8){
    if(mByteCount < DCC_MAX_BYTES) {
      mBytestream[mByteCount++] = (mBitstream & 0xFF);
      mBitstream >>= 8;
      mNumBits -= 8;
      return true;
    }
    else {
        resetBitstream();
    }
  }
  return false;
}

uint8_t DccSignalDecoder::getBytestream(char *bytestream) {
  addBitsToBitstream();
  evaluateBitstream();  
  
  if(mState == DCCPROTOCOL__PACKET_COMPLETE) {
    memcpy(bytestream, mBytestream, mByteCount);
    mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
    return mByteCount;
  }
  return 0;
}