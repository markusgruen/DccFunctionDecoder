#include "outputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>




#define DEBUG do{if(channel == 0){DccPacketHandler::confirmCvWrite();}}while(0)

uint16_t getPseudoRandomNumber();
uint8_t myRandomNumber(uint8_t min, uint8_t max);




// Volatile variables
volatile uint8_t vPwmValue[NUM_CHANNELS] = {0};
volatile uint8_t vRtcOverflowCounter; 

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
  bool channelState[4] = {0};

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

    // mode[0] = NEON;

    // dimmValue[0] = 127;
    // fadeSpeed[0] = 5;
    // blinkOnTime[0] = 200;
    // blinkOffTime[0] = 200;
  }

  // void update(Direction direction, uint32_t functions) {
  //   for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
  //     channelState[channel] = isChannelOn(functions, channel) && directionMatchesConfig(channel, direction);
  //   }
  // }

  void run(Direction direction, uint32_t functions) {  
    for(uint8_t channel = 0; channel < NUM_CHANNELS; channel++) {
      bool switch_on = isChannelOn(functions, channel) && directionMatchesConfig(channel, direction);

      switch(mode[channel]) {
        case FADE:
          fade(channel, switch_on);
          break;

        case BLINK:
          blinkStateMachine(channel, switch_on);
          break;

        case NEON:
          neonStateMachine(channel, switch_on);
          break;
      }
    }
  }

  enum BlinkStates{BLINK_OFF, BLINK_WAIT_OFF, BLINK_FADE_IN, BLINK_WAIT_ON, BLINK_FADE_OUT};
  void blinkStateMachine(uint8_t channel, bool switch_on) {
    static BlinkStates blinkState[NUM_CHANNELS] = {BLINK_OFF};

    if(switch_on) {
      switch(blinkState[channel]) {
        case BLINK_OFF:
          blinkState[channel] = BLINK_FADE_IN;
          break;
        
        case BLINK_FADE_IN:
          if(fade(channel, 1)) {
            blinkState[channel] = BLINK_WAIT_ON;
          }
          break;

        case BLINK_WAIT_ON:
          if(wait(channel, blinkOnTime[channel])) { // if wait_is_over
            blinkState[channel] = BLINK_FADE_OUT;
          }
          break;

        case BLINK_FADE_OUT:
          if(fade(channel, 0)) {
            blinkState[channel] = BLINK_WAIT_OFF;
          }
          break;
        
        case BLINK_WAIT_OFF:
          if(wait(channel, blinkOffTime[channel])) { // if wait_is_over
             blinkState[channel] = BLINK_FADE_IN;
             flackerCount[channel]++;
          }
          break;
      }
    }
    else { // if (switch_off)
      wait(channel, 0, true);  // reset wait
      if(fade(channel, 0)) {
        blinkState[channel] = BLINK_OFF;
      }
    }
  }

  enum NeonStates{NEON_OFF, NEON_FLICKER, NEON_FADE_IN, NEON_CONSTANT_ON};
  void neonStateMachine(uint8_t channel, bool switch_on) {
    static NeonStates neonState[NUM_CHANNELS] = {NEON_OFF};
    blinkOnTime[channel] = 2;

    if(switch_on) {
      switch(neonState[channel]) {
        case NEON_OFF:
          flackerCount[channel] = 0;
          fadeSpeed[channel] = 0;
          neonState[channel] = NEON_FLICKER;
          break;
        
        case NEON_FLICKER:
          if(flackerCount[channel] <= myRandomNumber(4, 20)) {
            blinkOffTime[channel] = myRandomNumber(32,160); // random(100,300)
            blinkStateMachine(channel, 1);
          }
          else {
            vPwmValue[channel] = dimmValue[channel]/16;
            fadeSpeed[channel] = 15; // langsames faden    
            neonState[channel] = NEON_FADE_IN;
          }
          break;

        case NEON_FADE_IN:
          if(fade(channel, 1)) {
            neonState[channel] = NEON_CONSTANT_ON;
          }
          break;

        case NEON_CONSTANT_ON:
          break;        
      }
    }

    else { // if(switch_off)
      fadeSpeed[channel] = 1;
      if(fade(channel, 0)) {
        neonState[channel] = NEON_OFF;
      }
    }
  }

  enum FadeStates{FADE_START, FADE_RUNNING};
  bool fade(uint8_t channel, bool fade_in) {
    static FadeStates fadeState[NUM_CHANNELS] = {FADE_START};
    static uint8_t nextEvent[NUM_CHANNELS] = {0};

    uint8_t targetDimmValue = fade_in ? dimmValue[channel] : 0;
    
    if(fadeSpeed[channel] == 0) {      
      vPwmValue[channel] = targetDimmValue;
      fadeState[channel] = FADE_START;
      return true;
    }
    else if(vPwmValue[channel] == targetDimmValue) {
      fadeState[channel] = FADE_START;
      return true;
    }
    else {
      if(fadeState[channel] == FADE_START) {
        nextEvent[channel] = vRtcOverflowCounter;
        fadeState[channel] = FADE_RUNNING;
      }

      if(fadeState[channel] == FADE_RUNNING) {
        uint8_t overflowCounter = vRtcOverflowCounter;
        if(overflowCounter == nextEvent[channel]) {
          nextEvent[channel] += fadeSpeed[channel];
          if (vPwmValue[channel] == targetDimmValue){ 
            fadeState[channel] = FADE_START;
            return true;
          }
          fade_in ? vPwmValue[channel]++ : vPwmValue[channel]--;
        }
      }
    }
    return false;
  }

  enum WaitStates{WAIT_START, WAIT_RUNNING};
  bool wait(uint8_t channel, uint8_t waitTime, bool reset){
    static uint8_t stopTime[NUM_CHANNELS] = {0};
    static uint8_t waitCounter[NUM_CHANNELS] = {0};
    
    if(reset){
      waitCounter[channel] = 0;
      return true;
    }

    if(waitCounter[channel] == 0) {
      stopTime[channel] = vRtcOverflowCounter + waitTime;
    }

    if (stopTime[channel] == vRtcOverflowCounter) {
      stopTime[channel] += waitTime;
      waitCounter[channel]++;
      if(waitCounter[channel] >= 10) {
        waitCounter[channel] = 0;
        return true;
      }
    }
    return false;
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