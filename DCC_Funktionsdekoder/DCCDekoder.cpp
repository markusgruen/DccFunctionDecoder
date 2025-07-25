#include "DCCDecoder.h"
#include <avr/interrupt.h>


enum class DccProtocolState : uint8_t {
  WAIT_FOR_PREAMBLE,   // Suche nach mindestens 10 Einsen
  WAIT_FOR_STARTBIT,   // Warte auf das Startbit (0)
  WAIT_FOR_ADDRESS,
  READ_DATA_BYTE,      // Lese 8 Datenbits + Trennerbit
  PACKET_COMPLETE,     // Paket vollständig empfangen
  ERROR                // Ungültiger Zustand oder Fehler
};



constexpr uint32_t US_TO_TICKS(uint32_t us) {
    return (us * (F_CPU / 1000000UL));
}

constexpr uint16_t DCC_MIN0_TICKS = US_TO_TICKS(52);
constexpr uint16_t DCC_MAX0_TICKS = US_TO_TICKS(64);
constexpr uint16_t DCC_MIN1_TICKS = US_TO_TICKS(100);
constexpr uint16_t DCC_MAX1_TICKS = US_TO_TICKS(200);



volatile int8_t vThisBit = 0;
volatile bool vPulseReceived = false;

DCCDecoder::DCCDecoder() {}

void DCCDecoder::begin(uint8_t pin) {
    // Pin als Eingang mit Pull-Up, falls nötig
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
}

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


void DCCDecoder::run(){
  
  if(vPulseReceived){
    noInterrupts();
    int8_t thisBit = vThisBit;
    vPulseReceived = false;
    interrupts();
    
    addBitToBitstream(thisBit);
  }
  evaluateBitstream();  
}

void DCCDecoder::resetBitstream(){
  mBitstream = 0;
  mValidBitCount = 0;
  mLastBit = -1;
}

void DCCDecder::addBitToBitstream(int8_t bit){
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

void evaluateBitstream() {
  switch(mState) {
    case DccProtocolState::WAIT_FOR_PREAMBLE:
      findPreamble();
      break;
    case DccProtocolState::WAIT_FOR_STARTBIT:
      findStartBit();
      break;
    case DccProtocolState::WAIT_FOR_ADDRESS:
      readAddressByte();
      break;
  }
}

void findPreamble() {
  while(mValidBitCount > 10) {
    while( !(mBitstream & 0x0000000000000001) && (mValidBitCount > 0) ) { // solange eine Null ganz rechts steht und noch Bits vorhanden sind
      mBitstream >>= 1;
      mValidBitCount--;
    }
    
    if(mValidBitCount > 10) {
      // prüfen, ob zehn Einsen ganz rechts stehen
      if( (mBitstream & 0x3FF) == 0x3FF) {
        mBitstream >>= 10; // alle zehn Einsen nach rechts herausschieben.
        mValidBitCount -= 10;
        mState = DccProtocolState::WAIT_FOR_STARTBIT;
        return;
      }
      else {
        // keine zehn Einsen --> bitstream um eines nach rechts schieben und weiter probieren.
        mBitstream >>= 1;
        mValidBitCount--;
      }
    }
  }
}

void findStartbit() {
  if(mValidBitCount >= 1) {
    if( (mBitstream & 0x01) == 0) { 
      mState = DccProtocolState::WAIT_FOR_ADDRESS;
    }
    mBitstream >>= 1;
    mValidBitCount--;
  }
}

void readAddressByte() {
  
}

