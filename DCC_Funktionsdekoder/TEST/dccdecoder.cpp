#include "dccdecoder.h"
#include <iostream>
#include <cstring>
#include <bitset>
//#include <Arduino.h>
//#include <avr/interrupt.h>

/*
constexpr uint32_t US_TO_TICKS(uint32_t us) {
    return (us * (F_CPU / 1000000UL));
}

constexpr uint16_t DCC_MIN0_TICKS = US_TO_TICKS(52);
constexpr uint16_t DCC_MAX0_TICKS = US_TO_TICKS(64);
constexpr uint16_t DCC_MIN1_TICKS = US_TO_TICKS(100);
constexpr uint16_t DCC_MAX1_TICKS = US_TO_TICKS(200);
*/


volatile int8_t vThisBit = 0;
volatile bool vPulseReceived = false;

DCCDecoder::DCCDecoder() {}

void DCCDecoder::begin(uint8_t pin) {
/*    // Pin als Eingang mit Pull-Up, falls nötig
    pinMode(pin, INPUT);

    // TCB0 auf Event-Capture-Modus einstellen:
    // - Count Mode: Capture (TCB_CNTMODE_CAPTURE_gc)
    // - Capture/Compare Enable (TCB_CCMPEN_bm)
    // - Capture Event Interrupt Enable (TCB_CAPTEI_bm)
    // - Clock Source: CLK_PER (div 1)

    // 1. Event system konfigurieren, Signal an TCB0 als Eventquelle koppeln
    // Eventchannel 0 mit Pin PA2 verbinden (Event Channel 0, PORTA Pin 2)
    // ATtiny402 Event-System:
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN2_gc;
    EVSYS.USER0 = EVSYS_USER0_TCB0_CAPT_gc; // TCB0 Capture als User0

    // 2. TCB0 konfigurieren
    TCB0.CTRLB = TCB_CNTMODE_CAPTURE_gc | TCB_CCMPEN_bm; // Capture Mode, CCMP Enable
    TCB0.EVCTRL = TCB_CAPTEI_bm;                         // Capture Interrupt Enable via Event
    TCB0.INTCTRL = TCB_CAPT_bm;                          // Interrupt bei Capture
    TCB0.CNT = 0;                                        // Timer zählen von 0
    TCB0.CTRLA = TCB_CLKSEL_DIV1_gc | TCB_ENABLE_bm;    // Taktquelle CLK_PER, Timer Enable

    // Wichtig: Im ISR musst du den Timer manuell zurücksetzen, da kein Auto-Reset möglich ist
  */
}

/*
ISR(TCB0_INT_vect) {
  uint16_t pulseLength = TCB0.CCMP;  // Timerstand kopieren
  if(pulseLength >= DCC_MIN0_TICKS && pulseLength <= DCC_MAX0_TICKS) {
    vThisBit = 0;
  }
  else if (pulseLength >= DCC_MIN1_TICKS && pulseLength <= DCC_MAX1_TICKS) {
    vThisBit = 1;
  }
  else {
    vThisBit = -1;
  }

  TCB0.CNT = 0;                     // Timer zurücksetzen
  vPulseReceived = true;               // Flag setzen
}

*/
void DCCDecoder::run(){
/*  
  if(vPulseReceived){
    noInterrupts();
    int8_t thisBit = vThisBit;
    vPulseReceived = false;
    interrupts();
    
    addBitToBitstream(thisBit);
  }
  */
  while(mState != DccProtocolState::PACKET_COMPLETE){
    evaluateBitstream();  
  }
}

void DCCDecoder::addBitToBitstream(int8_t bit){
  if (bit < 0) {
    resetBitstream();
    return;
  }

  if (mLastBit == -1) {
    // Erster Puls eines Paares speichern
    mLastBit = bit;
  } 
  else {
    // Zweiter Puls liegt vor
    if (bit == mLastBit) {
      // Gültiges Bitpaar → Bit in Stream einfügen
      //mBitstream = (mBitstream << 1) | bit;
      mBitstream = mBitstream | (bit << mValidBitCount);
      mValidBitCount++;
    }
    else {
      // Ungleiches Bitpaar → verwerfen
      resetBitstream();
    }

    mLastBit = -1; // immer zurücksetzen nach einem Paar
  }
}

void DCCDecoder::resetBitstream(){
  mBitstream = 0;
  mValidBitCount = 0;
  mLastBit = -1;
  mByteCount = 0;
  mNumOnesInPreamble = 0;
  mState = DccProtocolState::WAIT_FOR_PREAMBLE;
}

void DCCDecoder::evaluateBitstream() {
  int separator = -1;
  
  switch(mState) {
    case DccProtocolState::WAIT_FOR_PREAMBLE:
      if(findPreamble() == true){
        mState = DccProtocolState::READ_DATA_BYTE;
      }
      break;

    case DccProtocolState::WAIT_FOR_SEPARATOR:
      separator = getSeparator();
      if(separator== 0){
        mState = DccProtocolState::READ_DATA_BYTE;
      } else if(separator == 1){
        mState = DccProtocolState::PACKET_COMPLETE;
      } else { // no separator received yet, do nothing
      }
      break;

    case DccProtocolState::READ_DATA_BYTE:
      if(readDataByte() == true) {
        mState = DccProtocolState::WAIT_FOR_SEPARATOR;
      }
      break;

    case DccProtocolState::PACKET_COMPLETE:
      // will never be executed, see run();
      break;

    case DccProtocolState::ERROR:
      resetBitstream();
      mState = DccProtocolState::WAIT_FOR_PREAMBLE;
      break;
  }
}

bool DCCDecoder::findPreamble() {
    bool bit = 0;
    
    while(mValidBitCount > 0) {
        bit =(mBitstream & 0b1);
        mBitstream >>= 1;
        mValidBitCount--;
    
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

int8_t DCCDecoder::getSeparator() {
    
  int8_t separator = -1;
  if(mValidBitCount >= 1) {
    separator = (mBitstream & 0x01);
    mBitstream >>= 1;
    mValidBitCount--;
  }
  return separator;
}

bool DCCDecoder::readDataByte() {
  if(mValidBitCount >= 8){
    if(mByteCount < DCC_MAX_BYTES) {
      mBytestream[mByteCount++] = (mBitstream & 0xFF);
      mBitstream >>= 8;
      mValidBitCount -= 8;
      return true;
    }
    else {
        resetBitstream();
    }
  }
  return false;
}

void DCCDecoder::TESTsetBitstream(uint64_t bitstream, uint8_t numValidBits) {
  mBitstream = bitstream;
  std::bitset<64> x(mBitstream);
    std::cout << x << '\n';
  mValidBitCount = numValidBits;
}

uint8_t DCCDecoder::getBytestream(char *bytestream) {
      memcpy(bytestream, mBytestream, mByteCount);
      return mByteCount;
}
