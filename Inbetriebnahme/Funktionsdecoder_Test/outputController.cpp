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
  uint32_t functionMap[NUM_CHANNELS];
  uint8_t dimmValue[NUM_CHANNELS];
  uint8_t fadeSpeed[NUM_CHANNELS];
  uint8_t blinkOnTime[NUM_CHANNELS];
  uint8_t blinkOffTime[NUM_CHANNELS];
  Mode mode[NUM_CHANNELS];
  Direction direction;
  State channelState[4];

  void begin(){
    // configure output pins
    PORTA.DIRSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
    PORTA.OUTSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);


    // setup Timer A for software-PWM with >100Hz frequency at 8bit resolution
    // 20 MHz / 8 = 2.5 MHz → 1 Takt = 0.4 µs → 30 µs = 75 Takte
    TCB0.CCMP = 599;                   // 600 Takte @ 20 MHz = 30 µs
    TCB0.INTCTRL = TCB_CAPT_bm;        // Interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;        // Timer einschalten (kein Prescaler = CLK_PER)

    // setup RTC for 10 ms Interrupt
    RTC.PER = 15;                                      // period = 163 ticks ~= 5 ms
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

    mode[0] = NEON;

    dimmValue[0] = 127;
    dimmValue[1] = 127;
    fadeSpeed[0] = 20;
    fadeSpeed[1] = 5;
    blinkOnTime[0] = 200;
    blinkOffTime[0] = 200;
  }

  void update(Direction direction, uint8_t speed, uint32_t functions) {
    for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
      bool channelIsOn = isChannelOn(functions, channel);
      bool directionMatches = directionMatchesConfig(channel, direction);

      if(channelIsOn) {
        vPwmValue[channel] = 0;
        channelState[channel] = SWITCH_ON;
        flackerCount[channel] = 0;
      }
      else if(channelState[channel] == ON || channelState[channel] == TRANSITION_ON) {
        channelState[channel] = SWITCH_OFF;
      }

    }
  }

  void run() {
    static uint16_t nextEvent[NUM_CHANNELS] = {0};

    for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {

      if (channelState[channel] == SWITCH_ON) {  
        cli();
        __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
        nextEvent[channel] = vRtcOverflowCounter;
        __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
        sei();
        
        channelState[channel] = TRANSITION_ON;
      }
      else if (channelState[channel] == SWITCH_OFF) {
        cli();
        __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
        nextEvent[channel] = vRtcOverflowCounter;
        __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
        sei();

        channelState[channel] = TRANSITION_OFF;
      }

      switch(mode[channel]) {
        case FADE:    
          if(channelState[channel] == TRANSITION_ON) {
            if(fade(channel, &nextEvent[channel], FADE_IN)) {
              channelState[channel] = ON;
            }
          }
          else if(channelState[channel] == TRANSITION_OFF) {
            if(fade(channel, &nextEvent[channel], FADE_OUT)) {
              channelState[channel] = OFF;
            }
          }
          break;

        case BLINK:
          if(channelState[channel] == TRANSITION_ON) {
            blink(channel, &nextEvent[channel]);
          }
          else if(channelState[channel] == TRANSITION_OFF) {
            if(fade(channel, &nextEvent[channel], FADE_OUT)) {
              channelState[channel] = OFF;
            }
          }
          break;
          
        case NEON:
          if(channelState[channel] == TRANSITION_ON) {
            neon(channel, &nextEvent[channel]);
          }
          else if (channelState[channel] == TRANSITION_OFF) {
            fadeSpeed[channel] = 0;
            fade(channel, &nextEvent[channel], FADE_OUT);
          }
          break;
      }
    }


    // for(uint8_t channel=0; channel<NUM_CHANNELS; channel++){
    //   switch(mode[channel]) {
    //     case FADE:
    //       if(channelState[channel] == TRANSITION_ON) {
    //         if(fade(channel, &nextEvent[channel], FADE_IN)) {
    //           channelState[channel] = ON;
    //           DccPacketHandler::confirmCvWrite();
    //         }
    //       }
    //       if(channelState[channel] == TRANSITION_OFF) {
    //         if(fade(channel, &nextEvent[channel], FADE_OUT)) {
    //           channelState[channel] = OFF;
    //         }
    //       }
    //       break;
    //     case BLINK:
    //       blink(channel, &nextEvent[channel]);
    //       break;
    //     case NEON:
    //       neon(channel, &nextEvent[channel]);
    //       break;
    //   }
    // }
  }


  bool fade(uint8_t channel, uint16_t* nextEvent, bool up) {
    uint8_t targetDimmValue = up ? dimmValue[channel] : 0;

    if(fadeSpeed[channel] == 0) {      
      vPwmValue[channel] = targetDimmValue;
      return true;
    }
    else if(vPwmValue[channel] == targetDimmValue) {
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

  enum BlinkState{BLINK_FADE_IN, BLINK_ON, BLINK_FADE_OUT, BLINK_OFF};
  void blink(uint8_t channel, uint16_t* nextEvent) {
    static BlinkState blinkState[NUM_CHANNELS] = {BLINK_FADE_IN, BLINK_FADE_IN, BLINK_FADE_IN, BLINK_FADE_IN};

    // if(channelState[channel] == TRANSITION_ON) {
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
    // }
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

    if (channelState[channel] == TRANSITION_ON) {
      if(tubeState == FLICKER) {
        fadeSpeed[channel] = 0;

        if(flackerCount[channel] <= myRandomNumber(4, 20)) {  // random(8,13)
          blinkOffTime[channel] = myRandomNumber(32,160); // random(100,300)
          blink(channel, nextEvent);
        }
        else {
          vPwmValue[channel] = dimmValue[channel]/16;
          fadeSpeed[channel] = 150; // langsames faden
          tubeState = RAMP_UP;
        }
      }
      else { // tubestate == RAMP_UP
        if(fade(channel, nextEvent, FADE_IN)) {
          channelState[channel] = ON;
          tubeState == FLICKER;
        }
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

  PORTA.OUTSET = CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN;
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
      uint8_t val = vPwmValue[channel];  // lokale Kopie für Konsistenz
      if (sPwmCounter < val) {
          PORTA.OUTCLR = channelPin_bm[channel];  // LED an
      }
  }

  sPwmCounter++;
  TCB0.INTFLAGS = TCB_CAPT_bm;  // Interrupt-Flag löschen
}

ISR(RTC_CNT_vect) {
  vRtcOverflowCounter++;
  RTC.INTFLAGS = RTC_OVF_bm; // Flag löschen
}