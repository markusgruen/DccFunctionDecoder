#include "DccSignalParser.h"
#include "DccSignalReceiver_POLLING.h"
#include "DccPacketHandler.h"  // for "dccPacket[]"
#include "pinmap.h"


/**
 * @brief Reverse the bit order of a byte (LSB↔MSB)
 * @return Byte b with reversed bits
 */ 
inline uint8_t reverseByte(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;  // swap nibbles
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;  // swap bit pairs
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;  // swap single bits
    return b;
}


namespace DccSignalParser {
  // External flag 
  bool newDccPacket;

  // Internal variables
  uint64_t bitstream = 0;
  uint8_t numBitsReceived = 0;
  uint8_t packetByteCount = 0;
  uint8_t numOnesInPreamble = 0;
  uint8_t state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
  uint8_t byteStream[DCC_MAX_BYTES];

  /**
   * @brief Initialize DCC signal receiver
   */
  void begin(){
    DccSignalReceiver::begin(); 
  }

  /**
   * @brief Main parser routine
   *        Adds new bits to the bitstream and evaluates the protocol state machine
   */
  void run() {
    addBitsToBitstream();
    evaluateBitstream();  
  }

  /**
   * @brief Transfer bytes from ISR ring buffer into bitstream
   */
  void addBitsToBitstream() { 
    uint64_t tempBitstream = 0;

    while(1){  // there is a return in here...
      noInterrupts();
        uint8_t head = DccSignalReceiver::ringbuf.head;
        uint8_t tail = DccSignalReceiver::ringbuf.tail;
      interrupts();

      if (tail == head) {
        return; // Buffer empty
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

  
   /**
   * @brief Reset parser state and counters
   */
  void resetBitstream(){
    bitstream = 0;
    numBitsReceived = 0;
    packetByteCount = 0;
    numOnesInPreamble = 0;
    state = DCCPROTOCOL__WAIT_FOR_PREAMBLE;
  }

  /**
   * @brief Main DCC protocol state machine.
   * @details Called repeatedly in run(), switches parser state and saves packets when complete
   */
  void evaluateBitstream() {
    int separator = -1;
    
    switch(state) {
      case DCCPROTOCOL__WAIT_FOR_PREAMBLE:
        if(findPreamble() == true){
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

  /**
   * @brief Search for DCC preamble (≥10 ones followed by a zero)
   * @return true if valid preamble found, false otherwise
   */
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

  /**
   * @brief Get separator bit (start=0, stop=1)
   * @return 0 for start bit, 1 for stop bit, -1 if not enough bits
   */
  int8_t getSeparator() {
      
    int8_t separator = -1;
    if(numBitsReceived >= 1) {
      separator = (bitstream & 0b1);
      bitstream >>= 1;
      numBitsReceived--;
    }
    return separator;
  }

  /**
   * @brief Read one DCC data byte from bitstream
   * @return true if a full byte was read, false if not enough bits or buffer overflow
   */
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

  /**
   * @brief Verify XOR checksum (last byte) of packet
   * @return true if checksum is correct, false if error
   */
  bool checkErrorByteOK() {
    uint8_t errorByte = 0;
    for(uint8_t i=0; i<(packetByteCount-1); i++) {
      errorByte ^= byteStream[i];
    }
    return (errorByte == byteStream[packetByteCount-1]);
  }

  /**
   * @brief Save validated packet to global DCC buffer
   * @details Copies all bytes including error byte, updates packet size
   */
  void saveDccPacket() {
    memcpy(DccPacketHandler::dccPacket, byteStream, packetByteCount);  // the error-byte is copied too, because it is evaluted during CV programming
    DccPacketHandler::dccPacketSize = packetByteCount-1;
    newDccPacket = true;
  }
}
