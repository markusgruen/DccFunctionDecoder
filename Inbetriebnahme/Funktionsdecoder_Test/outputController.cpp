#include "outputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>


uint16_t getPseudoRandomNumber();
uint8_t myRandomNumber(uint8_t min, uint8_t max);

#define FADE_IN 1
#define FADE_OUT 0


// Volatile variables
volatile uint8_t vPwmValue[NUM_CHANNELS] = {0};
volatile uint16_t vRtcOverflowCounter; 

// Global variables
const uint8_t channelPin_bm[NUM_CHANNELS] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN};
uint8_t flackerCount[NUM_CHANNELS];


namespace OutputController{
  // Local variables
  // uint32_t functionMap[NUM_CHANNELS] = {0b00000010, 0b00000100, 0b00001000, 0b00010000};
  // uint8_t dimmValue[NUM_CHANNELS] = {127, 127, 127, 127};
  // uint8_t fadeSpeed[NUM_CHANNELS] = {0, 50, 50, 50};
  // uint8_t blinkOnTime[NUM_CHANNELS] = {2, 50, 50, 50};
  // uint8_t blinkOffTime[NUM_CHANNELS] = {200, 150, 150, 150};
  // Mode mode[NUM_CHANNELS] = {BLINK, FADE, FADE, FADE};
  // Direction direction = FORWARD;
  // State channelPinState[4] = {OFF, OFF, OFF, OFF};  
  uint32_t functionMap[NUM_CHANNELS];
  uint8_t dimmValue[NUM_CHANNELS];
  uint8_t fadeSpeed[NUM_CHANNELS];
  uint8_t blinkOnTime[NUM_CHANNELS];
  uint8_t blinkOffTime[NUM_CHANNELS];
  Mode mode[NUM_CHANNELS];
  Direction direction;
  State channelPinState[4];

  void begin(){
    // configure output pins
    PORTA.DIRSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
    PORTA.OUTSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);


    // setup Timer A for software-PWM with >100Hz frequency at 8bit resolution
    // 20 MHz / 8 = 2.5 MHz → 1 Takt = 0.4 µs → 30 µs = 75 Takte

    // TCB0.CTRLB = TCB_CNTMODE_INT_gc;   // Periodic Interrupt Mode  // <-- default 
    TCB0.CCMP = 599;                   // 600 Takte @ 20 MHz = 30 µs
    TCB0.INTCTRL = TCB_CAPT_bm;        // Interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;        // Timer einschalten (kein Prescaler = CLK_PER)
    // TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV8_gc;   // Clock = 20 MHz / 8 = 2.5 MHz
    // TCA0.SINGLE.PER = 74;                            // Compare match nach 75 Takten = 30 µs
    // TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;        // Compare match interrupt aktivieren
    // TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;       // Timer aktivieren

    // setup RTC for 10 ms Interrupt
    // RTC.CTRLA = 0;                                      // stop RTC
    // RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                  // select 32.768 Osc. as source  // <-- default 
    RTC.PER = 15;                                      // period = 163 ticks ~= 5 ms
    //RTC.INTFLAGS = RTC_OVF_bm;                          // clear Overflow flag
    RTC.INTCTRL = RTC_OVF_bm;                           // activate overflow interrupt
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc | RTC_RTCEN_bm;   // Prescaler 1, start RTC
  }

  void readCVs() {
    for(uint8_t i=0; i<NUM_CHANNELS; i++) {
    // unrolling does NOT save flash, it requires MORE space!
      functionMap[i]  = (uint32_t)EEPROM.read(ch1FunctionMap0 + (i*3));
      functionMap[i] |= (uint32_t)EEPROM.read(ch1FunctionMap1 + (i*3)) << 8;
      functionMap[i] |= (uint32_t)EEPROM.read(ch1FunctionMap2 + (i*3)) << 16;

      mode[i] = (Mode)EEPROM.read(ch1Mode + i);
      dimmValue[i] = EEPROM.read(ch1DimmValue + i);
      fadeSpeed[i] = EEPROM.read(ch1FadeSpeed + i);
      blinkOnTime[i] = EEPROM.read(ch1BlinkOnTime + i);
      blinkOffTime[i] = EEPROM.read(ch1BlinkOffTime + i);
    }

    dimmValue[0] = 127;
  }

  void update(Direction direction, uint8_t speed, uint32_t functions) {
    for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
      bool channelIsOn = isChannelOn(functions, channel);
      bool directionMatches = directionMatchesConfig(channel, direction);

      // if(channelIsOn && directionMatches) {
      if(channelIsOn) {
        vPwmValue[channel] = 0;
        channelPinState[channel] = TRANSITION_ON;
        flackerCount[channel] = 0;
        // if(channel==0) DccPacketHandler::confirmCvWrite();
      } 
      else {
        if(channelPinState[channel] == ON || channelPinState[channel] == TRANSITION_ON) {
          channelPinState[channel] = TRANSITION_OFF;
        }
      }
    }
  }

  void run() {
    static uint16_t nextEvent[NUM_CHANNELS] = {0};

    for(uint8_t channel=0; channel<NUM_CHANNELS; channel++){
      switch(mode[channel]) {
        case FADE:
          if(channelPinState[channel] == TRANSITION_ON) {
            // if(channel==0) DccPacketHandler::confirmCvWrite();
            // if(fade_in(channel, &nextEvent[channel])){
            if(fade(channel, &nextEvent[channel], FADE_IN)) {
              channelPinState[channel] = ON;
            }
          }
          if(channelPinState[channel] == TRANSITION_OFF) {
            if(fade(channel, &nextEvent[channel], FADE_OUT)) {
              channelPinState[channel] = OFF;
            }
          }
          break;
        case BLINK:
          blink(channel, &nextEvent[channel]);
          break;
        case NEON:
          neon(channel, &nextEvent[channel]);
          break;
      }
    }
  }


  bool fade(uint8_t channel, uint16_t* nextEvent, bool up) {
    uint8_t targetDimmValue = up ? dimmValue[channel] : 0;

    if(fadeSpeed[channel] == 0) {      
      vPwmValue[channel] = targetDimmValue;
      if(channel == 0) {
        up ? (PORTA.OUTCLR = CH1_PIN) : (PORTA.OUTSET = CH1_PIN);
      }
      // if(channel == 0 && vPwmValue[0] == 127) DccPacketHandler::confirmCvWrite();
      return true;
    }
    else {
      cli();
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      uint16_t overflowCounter = vRtcOverflowCounter;
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      sei();

      if(overflowCounter == *nextEvent) {
        *nextEvent += fadeSpeed[channel];
        up ? vPwmValue[channel]++ : vPwmValue[channel]--;
        if (vPwmValue[channel] == targetDimmValue){
          
          return true;
        }
      }
    }
    return false;


  }

  // bool fade_in(uint8_t channel, uint16_t* nextEvent) {
  //   if(fadeSpeed[channel] == 0) {
  //     vPwmValue[channel] = dimmValue[channel];
  //     return true;
  //   }
  //   else {
  //     cli();
  //     __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //     uint16_t overflowCounter = vRtcOverflowCounter;
  //     __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //     sei();

  //     if(overflowCounter == *nextEvent) {
  //       *nextEvent += fadeSpeed[channel];
  //       vPwmValue[channel]++;
  //       if (vPwmValue[channel] == dimmValue[channel]){
  //         return true;
  //       }
  //     }
  //   }
  //   return false;
  // }

  // bool fade_out(uint8_t channel, uint16_t* nextEvent) {
  //   if(fadeSpeed[channel] == 0) {
  //     vPwmValue[channel] = 0;
  //     return true;
  //   }
  //   else {
  //     cli();
  //     __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //     uint16_t overflowCounter = vRtcOverflowCounter;
  //     __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //     sei();

  //     if(overflowCounter == *nextEvent) {
  //       *nextEvent += fadeSpeed[channel];
  //       vPwmValue[channel]--;
  //       if (vPwmValue[channel] == 0){
  //         return true;
  //       }
  //     }
  //   }
  //   return false;
  // }

  enum BlinkState{BLINK_FADE_IN, BLINK_ON, BLINK_FADE_OUT, BLINK_OFF};
  void blink(uint8_t channel, uint16_t* nextEvent) {
    static BlinkState blinkState[NUM_CHANNELS] = {BLINK_FADE_IN, BLINK_FADE_IN, BLINK_FADE_IN, BLINK_FADE_IN};

    if(channelPinState[channel] == TRANSITION_ON) {
      switch(blinkState[channel]) {
        case BLINK_FADE_IN:
          if(fade(channel, nextEvent, FADE_IN)) {
            *nextEvent += 10*blinkOnTime[channel];
            blinkState[channel] = BLINK_ON;
          }
          break;
        case BLINK_ON:
          if(wait(channel, nextEvent)) {
            blinkState[channel] = BLINK_FADE_OUT;
          }
          break;
        case BLINK_FADE_OUT:
          if(fade(channel, nextEvent, FADE_OUT)) {
            *nextEvent += 10*blinkOffTime[channel];
            blinkState[channel] = BLINK_OFF;
          }
          break;
        case BLINK_OFF:
          if(wait(channel, nextEvent)) {
            blinkState[channel] = BLINK_FADE_IN;
            flackerCount[channel]++;
          }
          break;
      }
    }
  }

  bool wait(uint8_t channel, uint16_t* nextEvent){
    cli();
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    uint16_t overflowCounter = vRtcOverflowCounter;
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    sei();

    if(overflowCounter == *nextEvent) {
      return true;
    }
    return false;
  }

  enum TubeState{FLICKER, RAMP_UP};
  void neon(uint8_t channel, uint16_t* nextEvent) {
    static TubeState tubeState = FLICKER;
    blinkOnTime[channel] = 2;
    fadeSpeed[channel] = 0;

    if (channelPinState[channel] == TRANSITION_ON) {
      if(tubeState == FLICKER) {
        if(flackerCount[channel] <= myRandomNumber(4, 20)) {  // random(8,13)
          blinkOffTime[channel] = myRandomNumber(32,160); // random(100,300)
          blink(channel, nextEvent);
        }
        else {
          tubeState = RAMP_UP;
        }
      }
      else { // tubestate == RAMP_UP
        vPwmValue[channel] = dimmValue[channel]/16;
        fadeSpeed[channel] = 150; // langsames faden
        mode[channel] = FADE;
      }
    }
  }

  bool forwardOnly(uint8_t channel) {
    return bit_is_set(functionMap[channel], 30);
  }
  
  bool reverseOnly(uint8_t channel) {
    return bit_is_set(functionMap[channel], 31);
  }

  bool isChannelOn(uint32_t functions, uint8_t channel) {
    return functions & functionMap[channel]; 
  }
  
  bool directionMatchesConfig(uint8_t channel, Direction direction) {
    if(forwardOnly(channel)) return direction == FORWARD;
    if(reverseOnly(channel)) return direction == REVERSE;
    return true; 
  }
};


uint16_t lfsr16 = 0xACE1;
uint16_t getPseudoRandomNumber() {
  // Xorshift-basierter 16-bit LFSR
  lfsr16 ^= lfsr16 << 7;
  lfsr16 ^= lfsr16 >> 9;
  lfsr16 ^= lfsr16 << 8;
  return lfsr16;
}

uint8_t myRandomNumber(uint8_t min, uint8_t max){
  return (getPseudoRandomNumber() % (max - min)) + min;
}


ISR(TCB0_INT_vect) {
  static uint8_t sPwmCounter = 0;

  // if(vPwmValue[0] == 0) DccPacketHandler::confirmCvWrite();
  // vPwmValue[0] = 127;

  for (uint8_t channel=1; channel<NUM_CHANNELS; channel++) {
    if (sPwmCounter < vPwmValue[channel]) {
      PORTA.OUTCLR = channelPin_bm[channel];  // LOW (active)
    }
    else {
      PORTA.OUTSET = channelPin_bm[channel];  // HIGH
    }
  }

  // // Geringfügig mehr flash, aber schneller(?)
  // uint8_t clrMask = 0;
  // uint8_t setMask = 0;

  // if (sPwmCounter < vPwmValue[0]) clrMask |= CH1_PIN; else setMask |= CH1_PIN;
  // if (sPwmCounter < vPwmValue[1]) clrMask |= CH2_PIN; else setMask |= CH2_PIN;
  // if (sPwmCounter < vPwmValue[2]) clrMask |= CH3_PIN; else setMask |= CH3_PIN;
  // if (sPwmCounter < vPwmValue[3]) clrMask |= CH4_PIN; else setMask |= CH4_PIN;

  // PORTA.OUTCLR = clrMask;   // aktive Kanäle einschalten
  // PORTA.OUTSET = setMask;   // inaktive ausschalten


  sPwmCounter++;
  TCB0.INTFLAGS = TCB_CAPT_bm;  // Interrupt-Flag löschen
}

ISR(RTC_CNT_vect) {
  vRtcOverflowCounter++;
  RTC.INTFLAGS = RTC_OVF_bm; // Flag löschen
}