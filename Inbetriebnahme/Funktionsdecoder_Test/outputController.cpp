#include "outputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>


uint16_t getPseudoRandomNumber();


// Volatile variables
volatile uint8_t vPwmValue[NUM_CHANNELS] = {0};
volatile uint16_t vRtcOverflowCounter; 

// Global variables
const uint8_t channelPin_bm[NUM_CHANNELS] = {CH1_PIN, CH2_PIN, CH3_PIN, CH4_PIN};
uint8_t flackerCount;


namespace OutputController{
  // Local variables
  uint32_t functionMap[NUM_CHANNELS];
  uint8_t dimmValue[NUM_CHANNELS] = {127, 127, 127, 127};
  uint8_t fadeSpeed[NUM_CHANNELS] = {50, 50, 50, 50};
  uint8_t blinkOnTime[NUM_CHANNELS] = {50, 50, 50, 50};
  uint8_t blinkOffTime[NUM_CHANNELS] = {150, 150, 150, 150};
  Mode mode[NUM_CHANNELS] = {BLINK, BLINK, BLINK, BLINK};
  Direction direction = FORWARD;
  State channelPinState[4] = {OFF, OFF, OFF, OFF};  

  void begin(){
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
    RTC.PER = 33;                                      // period = 163 ticks ~= 5 ms
    //RTC.INTFLAGS = RTC_OVF_bm;                          // clear Overflow flag
    RTC.INTCTRL = RTC_OVF_bm;                           // activate overflow interrupt
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc | RTC_RTCEN_bm;   // Prescaler 1, start RTC

    // configure output pins

    PORTA.DIRSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
    PORTA.OUTSET = (CH1_PIN | CH2_PIN | CH3_PIN | CH4_PIN);
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
  }

  void update(Direction direction, uint8_t speed, uint32_t functions) {
    for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
      bool channelIsOn = isChannelOn(functions, channel);
      bool directionMatches = directionMatchesConfig(channel, direction);

      if(channelIsOn && directionMatches) {
        vPwmValue[channel] = 0;
        channelPinState[channel] = TRANSITION_ON;
      } else {
        channelPinState[channel] = TRANSITION_OFF;
      }
    }
  }

  void run() {
    static uint16_t nextEvent[NUM_CHANNELS] = {0};

    for(uint8_t channel=0; channel<NUM_CHANNELS; channel++){
      switch(mode[channel]) {
        case FADE:
          if(channelPinState[channel] == TRANSITION_ON) {
            fade_in(channel, &nextEvent[channel]);
          }
          if(channelPinState[channel] == TRANSITION_OFF) {
            fade_out(channel, &nextEvent[channel]);
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

  // void switchOn(uint8_t channel) {

  //   if(channelPinState[channel] == OFF || channelPinState[channel] == TRANSITION_OFF) {
  //     switch(mode[channel]) {
  //       case ONOFF:
  //         vPwmValue[channel] = dimmValue[channel];
  //         channelPinState[channel] = ON;  
  //         break;
  //       case FADE:
  //         //vPwmValue[channel] = 0;
  //         channelPinState[channel] = TRANSITION_ON;
  //         break;
  //       case BLINK:
  //         //vPwmValue[channel] = 0;
  //         // channelPinState[channel] = ON;
  //         channelPinState[channel] = TRANSITION_ON;
  //         break;
  //       case NEON:
  //         channelPinState[channel] = TRANSITION_ON;
  //         break;
  //     }
  //   }
  // }

  // void switchOff(uint8_t channel) {
  //   if(channelPinState[channel] == ON || channelPinState[channel] == TRANSITION_ON) {
  //     switch(mode[channel]) {
  //       case ONOFF:
  //         vPwmValue[channel] = 0;
  //         channelPinState[channel] = OFF;  
  //         break;
  //       case FADE:
  //         channelPinState[channel] = TRANSITION_OFF;
  //         break;
  //       case BLINK:
  //         vPwmValue[channel] = 0;
  //         channelPinState[channel] = OFF;
  //         break;
  //       case NEON:
  //         vPwmValue[channel] = 0;
  //         mode[channel] = NEON; // gets set to "FADE" during neon-startup
  //         channelPinState[channel] = OFF;
  //         break;
  //     }
  //   }
  // }

  bool fade_in(uint8_t channel, uint16_t* nextEvent) {
    if (channelPinState[channel] == TRANSITION_ON) {
      cli();
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      uint16_t overflowCounter = vRtcOverflowCounter;
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      sei();

      if(overflowCounter == *nextEvent) {
        *nextEvent += fadeSpeed[channel];
        vPwmValue[channel]++;
        if (vPwmValue[channel] == dimmValue[channel]){
          channelPinState[channel] = ON;
          return true;
        }
      }
    }
    return false;
  }

  bool fade_out(uint8_t channel, uint16_t* nextEvent) {
    if (channelPinState[channel] == TRANSITION_OFF) {
      cli();
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      uint16_t overflowCounter = vRtcOverflowCounter;
      __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
      sei();

      if(overflowCounter == *nextEvent) {
        *nextEvent += fadeSpeed[channel];
        vPwmValue[channel]--;
        if (vPwmValue[channel] == 0){
          channelPinState[channel] = OFF;
          return true;
        }
      }
    }
    return false;
  }


  void blink(uint8_t channel, uint16_t* nextEvent) {
    switch (channelPinState[channel]) {
      case TRANSITION_ON:
        fade_in(channel, nextEvent);
        // if(fade_in(channel, nextEvent)) {
        //   *nextEvent += 10*blinkOnTime[channel];
        // }
        break;
      case ON:
        // wait(channel, nextEvent, 1);
        break;
      case TRANSITION_OFF:
        // if(fade_out(channel, nextEvent)) {
        //   *nextEvent += 10*blinkOffTime[channel];
        //   vPwmValue[channel] = 0;
        // }
        break;
      case OFF:
        // wait(channel, nextEvent, 0);
        break;
    }
  }

  void wait(uint8_t channel, uint16_t* nextEvent, bool isOn){
    State nextState = isOn ? TRANSITION_OFF : TRANSITION_ON;

    cli();
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    uint16_t overflowCounter = vRtcOverflowCounter;
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    sei();

    if(overflowCounter == *nextEvent) {
      channelPinState[channel] = nextState;
      // flackerCount++;
    }
  }



  //   bool isOn = (bool)vPwmValue[channel];
  //   uint8_t waitTime = isOn ? blinkOffTime[channel] : blinkOnTime[channel];
  //   uint8_t nextValue = isOn ? 0 : dimmValue[channel]; 

  //   cli();
  //   __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //   uint16_t overflowCounter = vRtcOverflowCounter;
  //   __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  //   sei();

  //   if(overflowCounter == *nextEvent) {
  //     *nextEvent += 10*waitTime;
  //     vPwmValue[channel] = nextValue;
  //     flackerCount++;
  //   }
  // }

  enum TubeState{FLICKER, RAMP_UP};
  void neon(uint8_t channel, uint16_t* nextEvent) {
    static TubeState tubeState = FLICKER;
    blinkOnTime[channel] = 1;

    if (channelPinState[channel] == TRANSITION_ON) {
      if(tubeState == FLICKER){
        if(flackerCount <= ((getPseudoRandomNumber() % 6) + 8)) {  // random(6,11)
          blinkOffTime[channel] = ((getPseudoRandomNumber() % 80) + 20); // random(100,300)
          blink(channel, nextEvent);
        }
        else {
          tubeState = RAMP_UP;
        }
      }
      else {
        vPwmValue[channel] = dimmValue[channel]/16;
        fadeSpeed[channel] = 150; // langsames faden
        mode[channel] = FADE;
      }
    }


    // static TubeState tubeState = FLICKER;
    // static uint8_t sCounter = 0;
    // uint8_t oldvPwmValue[NUM_CHANNELS];
    // oldvPwmValue[channel] = vPwmValue[channel];

    // if (mChannelPinState[channel] == TRANSITION_ON) {
    //   if(vRtcOverflowCounter == *nextEvent) {
    //     if(tubeState == FLICKER) {      
    //       if(sCounter++ > ((getPseudoRandomNumber() % 5) + 6)) {  // random(6,11)
    //         vPwmValue[channel] = mDimmValue[channel];
    //         *nextEvent += ((getPseudoRandomNumber() % 200) + 100);  // random(100,300)
    //         tubeState = RAMP_UP;
    //       }
    //       else if(oldvPwmValue[channel] == 0) {
    //         vPwmValue[channel] = mDimmValue[channel];
    //         *nextEvent += ((getPseudoRandomNumber() % 70) + 30); // random(30,100)
    //       }
    //       else{
    //         vPwmValue[channel] = 0;
    //         *nextEvent += ((getPseudoRandomNumber() % 150) + 50); // random(50,200)
    //       }        
    //     }
    //     else if(tubeState == RAMP_UP) {
    //       vPwmValue[channel] = mDimmValue[channel]/2;
    //       mMultiFunctionValue[channel] = 30; // langsames faden
    //       mMode[channel] = FADE;
    //     }
    //   }
    // }
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


ISR(TCB0_INT_vect) {
  static uint8_t sPwmCounter = 0;

  for (uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
    if (sPwmCounter < vPwmValue[channel]) {
      PORTA.OUTSET = channelPin_bm[channel];  // High
    } else {
      PORTA.OUTCLR = channelPin_bm[channel];  // Low
    }
  }
  // uint8_t setMask = 0;
  // if(sPwmCounter < vPwmValue[0]) setMask |= CH1_PIN;
  // if(sPwmCounter < vPwmValue[1]) setMask |= CH2_PIN;
  // if(sPwmCounter < vPwmValue[2]) setMask |= CH3_PIN;
  // if(sPwmCounter < vPwmValue[3]) setMask |= CH4_PIN;
  // PORTA.OUT = (PORTA.OUT & ~(CH1_PIN|CH2_PIN|CH3_PIN|CH4_PIN)) | setMask;


  sPwmCounter++;
  TCB0.INTFLAGS = TCB_CAPT_bm;  // Interrupt-Flag löschen
}

ISR(RTC_CNT_vect) {
  vRtcOverflowCounter++;
  RTC.INTFLAGS = RTC_OVF_bm; // Flag löschen
}