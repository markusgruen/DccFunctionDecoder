#include "DccSignalParser.h"
#include "DccSignalReceiver_POLLING.h"


DccSignalReceiver receiver;


DccSignalParser::DccSignalParser() {
}

void DccSignalParser::begin(int pin, char* dccPacket, uint8_t* packetSize, bool* newDccPacket){
  mDccPacket = dccPacket;
  mPacketSize = packetSize;
  mNewDccPacket = newDccPacket;
  
  receiver.begin(pin);
}

void DccSignalParser::run() {
  addBitsToBitstream();
  evaluateBitstream();  
}

void DccSignalParser::addBitsToBitstream() {
  uint8_t newBitstreamByte = 0;
  uint64_t tempBitstream = 0;
  
  while(receiver.getNewBitstreamByte(&newBitstreamByte)) {
    tempBitstream = newBitstreamByte;
    tempBitstream <<= mNumBits;
    mBitstream |= tempBitstream;
    mNumBits += 8;

    // mBitstream |= (newBitstreamByte << 8);
    // mNumBits += 8;
  }
}

void DccSignalParser::resetBitstream(){
  mBitstream = 0;
  mNumBits = 0;
  mByteCount = 0;
  mNumOnesInPreamble = 0;
  mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
}

void DccSignalParser::evaluateBitstream() {
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

    case DCCPROTOCOL__PACKET_VALID:
      if(checkErrorByteOK()) {
        saveDccPacket();
        mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
      }
      else {
        mState = DCCPROTOCOL__ERROR;
      }
      break;

    case DCCPROTOCOL__ERROR:
      resetBitstream();
      mState = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
      break;
  }
}

bool DccSignalParser::findPreamble() {
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

int8_t DccSignalParser::getSeparator() {
    
  int8_t separator = -1;
  if(mNumBits >= 1) {
    separator = (mBitstream & 0b1);
    mBitstream >>= 1;
    mNumBits--;
  }
  return separator;
}

bool DccSignalParser::readDataByte() {
  if(mNumBits >= 8){
    if(mByteCount < DCC_MAX_BYTES) {
      mByteStream[mByteCount++] = (mBitstream & 0xFF);
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

bool DccSignalParser::checkErrorByteOK() {
  uint8_t errorByte = 0;
  for(uint8_t i=0; i<(mByteCount-1); i++) {
    errorByte ^= mByteStream[i];
  }
  return (errorByte == mByteStream[mByteCount]);
}

void DccSignalParser::saveDccPacket() {
  memcpy(mDccPacket, mByteStream, mByteCount-1);  // -1, because last byte is error-byte
  *mPacketSize = mByteCount-1;
  *mNewDccPacket = true;
}


