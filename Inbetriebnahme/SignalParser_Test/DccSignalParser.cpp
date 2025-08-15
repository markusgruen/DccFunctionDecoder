#include "DccSignalParser.h"
#include "DccSignalReceiver_POLLING.h"
#include "pinmap.h"
#include "debug_uart.h"


inline uint8_t reverseByte(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;  // Nibbles tauschen
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;  // 2-Bit-Paare tauschen
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;  // einzelne Bits tauschen
    return b;
}


namespace DccSignalParser {
  // extern variables 
  char dccPacket[];
  uint8_t dccPacketSize;
  bool newDccPacket;

  // internal variables
  uint64_t bitstream = 0;
  uint8_t numBitsReceived = 0;
  uint8_t packetByteCount = 0;
  uint8_t numOnesInPreamble = 0;
  uint8_t state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
  uint8_t byteStream[DCC_MAX_BYTES];

  // Functions

  void begin(){
    DccSignalReceiver::begin();
  }

  void run() {
    addBitsToBitstream();
    evaluateBitstream();  
  }

  void addBitsToBitstream() { 
    uint64_t tempBitstream = 0;

    while(1){  // there is a return in here...
      noInterrupts();
        uint8_t head = DccSignalReceiver::ringbuf.head;
        uint8_t tail = DccSignalReceiver::ringbuf.tail;
      interrupts();

      if (tail == head) {
        return; // Buffer leer
      }

      tempBitstream = DccSignalReceiver::ringbuf.buffer[tail++];
      tempBitstream <<= numBitsReceived;
      bitstream |= tempBitstream;
      numBitsReceived += 8;
      noInterrupts();
        DccSignalReceiver::ringbuf.tail = (tail & RINGBUF_MASK);
      interrupts();
    }
    return;  // wird nie erreicht...
  }

  void resetBitstream(){
    bitstream = 0;
    numBitsReceived = 0;
    packetByteCount = 0;
    numOnesInPreamble = 0;
    state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
  }

  void evaluateBitstream() {
    int separator = -1;
    
    switch(state) {
      case DCCPROTOCOL__WAIT_FOR_PREAMBLE:
        if(findPreamble() == true){
          PORTA.OUTSET = CH1_PIN;
          state = DCCPROTOCOL__READ_DATA_BYTE;
        }
        break;

      case DCCPROTOCOL__READ_DATA_BYTE:
        if(readDataByte() == true) {
          state = DCCPROTOCOL__WAIT_FOR_SEPARATOR;
        }
        break;

      case DCCPROTOCOL__WAIT_FOR_SEPARATOR:
        separator = getSeparator();
        if(separator == 0) {
          state = DCCPROTOCOL__READ_DATA_BYTE;
        }
        else if(separator == 1) {
          state = DCCPROTOCOL__PACKET_COMPLETE;
        }
        else { // no separator received yet, do nothing
        }
        break;

      case DCCPROTOCOL__PACKET_COMPLETE:
        if(checkErrorByteOK()) {
          PORTA.OUTCLR = CH1_PIN;
          saveDccPacket();
          state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
        }
        else {
          state = DCCPROTOCOL__ERROR;
        }

        // reset packtByteCounter
        packetByteCount = 0;
        break;

      case DCCPROTOCOL__ERROR:
        resetBitstream();
        state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
        break;
    }
  }

  bool findPreamble() {
    bool bit = 0;
    
    while(numBitsReceived > 0) {
      bit = bitstream & 0x01;
      bitstream >>= 1;
      numBitsReceived--;

      if(bit) {
        numOnesInPreamble++;
      }
      else {
        if(numOnesInPreamble >= 10) {
          // zero was found and >= 10 ones before this zero
          numOnesInPreamble = 0;
          return true;
        }
        else {
          // zero was found but not enough ones
          numOnesInPreamble = 0;
        }
      }
    }
    return false;
  }

  int8_t getSeparator() {
      
    int8_t separator = -1;
    if(numBitsReceived >= 1) {
      separator = (bitstream & 0b1);
      bitstream >>= 1;
      numBitsReceived--;
    }
    return separator;
  }

  bool readDataByte() {
    if(numBitsReceived >= 8){
      if(packetByteCount < DCC_MAX_BYTES) {
        byteStream[packetByteCount++] = reverseByte(bitstream & 0xFF); 
        bitstream >>= 8;
        numBitsReceived -= 8;
        return true;
      }
      else {
          resetBitstream();
      }
    }
    return false;
  }

  bool checkErrorByteOK() {
    uint8_t errorByte = 0;
    for(uint8_t i=0; i<(packetByteCount-1); i++) {
      errorByte ^= byteStream[i];
    }
    return (errorByte == byteStream[packetByteCount-1]);
  }

  void saveDccPacket() {
    memcpy(dccPacket, byteStream, packetByteCount-1);  // -1, because last byte is error-byte
    dccPacketSize = packetByteCount-1;
    newDccPacket = true;
  }
}
