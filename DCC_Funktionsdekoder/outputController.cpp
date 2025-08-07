#include "outputController.h"
#include "CV_table.h"
#include <Arduino.h>
#include <EEPROM.h>


// Global variables
// volatile uint16_t rtcOverflowCounter = 0;
uint16_t rtcOverflowCounter;
volatile uint8_t OutputController::pwmValues[NUM_CHANNELS] = {0};
uint8_t OutputController::mChannelPin_bm[NUM_CHANNELS] = {0};


uint16_t lfsr16 = 0xACE1;
uint16_t lfsr16_next() {
  // Xorshift-basierter 16-bit LFSR
  lfsr16 ^= lfsr16 << 7;
  lfsr16 ^= lfsr16 >> 9;
  lfsr16 ^= lfsr16 << 8;
  return lfsr16;
}



OutputController::OutputController(){
/*
  // DELETE
  mChannelPinState[0] = OFF;
  mChannelPinState[1] = OFF;
  mChannelPinState[2] = OFF;
  mChannelPinState[3] = OFF;

  mMode[0] = BLINK;
  mMode[1] = ONOFF;
  mMode[2] = ONOFF;
  mMode[3] = ONOFF;

  mFunctionMap[0] = 0b00000010;
  mFunctionMap[1] = 0b00000100;
  mFunctionMap[2] = 0b00001000;
  mFunctionMap[3] = 0b00010000;

  mSpeedThreshold[0] = 255;
  mSpeedThreshold[1] = 255;
  mSpeedThreshold[2] = 255;
  mSpeedThreshold[3] = 255;

  mDimmValue[0] = 127;
  mDimmValue[1] = 127;
  mDimmValue[2] = 127;
  mDimmValue[3] = 127;

  mFadeSpeed[0] = 10;
  mFadeSpeed[1] = 10;
  mFadeSpeed[2] = 10;
  mFadeSpeed[3] = 10;

  mBlinkOnTime[0] = 1;
  mBlinkOnTime[1] = 20;
  mBlinkOnTime[2] = 20;
  mBlinkOnTime[3] = 20;

  mBlinkOffTime[0] = 127;
  mBlinkOffTime[1] = 127;
  mBlinkOffTime[2] = 127;
  mBlinkOffTime[3] = 127;
*/
}

void OutputController::begin(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4){

  // setup Timer A for software-PWM with >100Hz frequency at 8bit resolution
  // 20 MHz / 8 = 2.5 MHz → 1 Takt = 0.4 µs → 30 µs = 75 Takte
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV8_gc;   // Clock = 20 MHz / 8 = 2.5 MHz
  TCA0.SINGLE.PER = 74;                            // Compare match nach 75 Takten = 30 µs
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;        // Compare match interrupt aktivieren
  TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;       // Timer aktivieren

  // setup RTC for 10 ms Interrupt
  // RTC.CTRLA = 0;                                      // stop RTC
  RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;                  // select 32.768 Osc. as source
  RTC.PER = 33;                                      // period = 163 ticks ~= 5 ms
  //RTC.INTFLAGS = RTC_OVF_bm;                          // clear Overflow flag
  RTC.INTCTRL = RTC_OVF_bm;                           // activate overflow interrupt
  RTC.CTRLA = RTC_PRESCALER_DIV1_gc | RTC_RTCEN_bm;   // Prescaler 1, start RTC

  // configure output pins
  mChannelPin_bm[0] = (1 << pin1);
  mChannelPin_bm[1] = (1 << pin2);
  mChannelPin_bm[2] = (1 << pin3);
  mChannelPin_bm[3] = (1 << pin4);

  PORTA.DIRSET |= mChannelPin_bm[0];
  PORTA.DIRSET |= mChannelPin_bm[1];
  PORTA.DIRSET |= mChannelPin_bm[2];
  PORTA.DIRSET |= mChannelPin_bm[3];
}

void OutputController::readCVs() {
  mFunctionMap[0]  = (uint32_t)EEPROM.read(ch1FunctionMap0);
  mFunctionMap[0] |= (uint32_t)EEPROM.read(ch1FunctionMap1) << 8;
  mFunctionMap[0] |= (uint32_t)EEPROM.read(ch1FunctionMap2) << 16;

  mFunctionMap[1]  = (uint32_t)EEPROM.read(ch2FunctionMap0);
  mFunctionMap[1] |= (uint32_t)EEPROM.read(ch2FunctionMap1) << 8;
  mFunctionMap[1] |= (uint32_t)EEPROM.read(ch2FunctionMap2) << 16;

  mFunctionMap[2]  = (uint32_t)EEPROM.read(ch3FunctionMap0);
  mFunctionMap[2] |= (uint32_t)EEPROM.read(ch3FunctionMap1) << 8;
  mFunctionMap[2] |= (uint32_t)EEPROM.read(ch3FunctionMap2) << 16;

  mFunctionMap[3]  = (uint32_t)EEPROM.read(ch4FunctionMap0);
  mFunctionMap[3] |= (uint32_t)EEPROM.read(ch4FunctionMap1) << 8;
  mFunctionMap[3] |= (uint32_t)EEPROM.read(ch4FunctionMap2) << 16;

  mSpeedThreshold[0] = EEPROM.read(ch1SpeedThreshold);
  mSpeedThreshold[1] = EEPROM.read(ch2SpeedThreshold);
  mSpeedThreshold[2] = EEPROM.read(ch3SpeedThreshold);
  mSpeedThreshold[3] = EEPROM.read(ch4SpeedThreshold);

  mDimmValue[0] = EEPROM.read(ch1DimmValue);
  mDimmValue[1] = EEPROM.read(ch2DimmValue);
  mDimmValue[2] = EEPROM.read(ch3DimmValue);
  mDimmValue[3] = EEPROM.read(ch4DimmValue);

  mMode[0] = (Mode)EEPROM.read(ch1Mode1);
  mMode[1] = (Mode)EEPROM.read(ch2Mode1);
  mMode[2] = (Mode)EEPROM.read(ch3Mode1);
  mMode[3] = (Mode)EEPROM.read(ch4Mode1);

  mFadeSpeed[0] = EEPROM.read(ch1FadeSpeed);
  mFadeSpeed[1] = EEPROM.read(ch2FadeSpeed);
  mFadeSpeed[2] = EEPROM.read(ch3FadeSpeed);
  mFadeSpeed[3] = EEPROM.read(ch4FadeSpeed);

  mBlinkOnTime[0] = EEPROM.read(ch1BlinkOnTime);
  mBlinkOnTime[1] = EEPROM.read(ch2BlinkOnTime);
  mBlinkOnTime[2] = EEPROM.read(ch3BlinkOnTime);
  mBlinkOnTime[3] = EEPROM.read(ch4BlinkOnTime);

  mBlinkOffTime[0] = EEPROM.read(ch1BlinkOffTime);
  mBlinkOffTime[1] = EEPROM.read(ch2BlinkOffTime);
  mBlinkOffTime[2] = EEPROM.read(ch3BlinkOffTime);
  mBlinkOffTime[3] = EEPROM.read(ch4BlinkOffTime);
}

void OutputController::update(Direction direction, uint8_t speed, uint32_t functions) {
  for(uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
    bool channelIsOn = isChannelOn(functions, channel);
    bool directionMatches = directionMatchesConfig(channel, direction);
    bool speedBelowThreshold = IsSpeedBelowLimit(channel, speed);

    if(channelIsOn && directionMatches && speedBelowThreshold) {
      switchOn(channel);
    } else {
      switchOff(channel);
    }
  }
}

void OutputController::run() {
  static uint16_t nextEvent[NUM_CHANNELS] = {0};

  for(uint8_t channel=0; channel<NUM_CHANNELS; channel++){
    switch(mMode[channel]) {
      case FADE:
        fade(channel, &nextEvent[channel]);
        break;
      case BLINK:
        if(mChannelPinState[channel] == ON) {
          blink(channel, &nextEvent[channel]);
        }
        break;
      case NEON:
        neon(channel, &nextEvent[channel]);
        break;
    }
  }
}

void OutputController::switchOn(uint8_t channel) {

  if(mChannelPinState[channel] == OFF || mChannelPinState[channel] == TRANSITION_OFF) {
    switch(mMode[channel]) {
      case ONOFF:
        pwmValues[channel] = mDimmValue[channel];
        mChannelPinState[channel] = ON;  
        break;
      case FADE:
        //pwmValues[channel] = 0;
        mChannelPinState[channel] = TRANSITION_ON;
        break;
      case BLINK:
        //pwmValues[channel] = 0;
        mChannelPinState[channel] = ON;
        break;
      case NEON:
        mChannelPinState[channel] = TRANSITION_ON;
        break;
    }
  }
}

void OutputController::switchOff(uint8_t channel) {
  if(mChannelPinState[channel] == ON || mChannelPinState[channel] == TRANSITION_ON) {
    switch(mMode[channel]) {
      case ONOFF:
        pwmValues[channel] = 0;
        mChannelPinState[channel] = OFF;  
        break;
      case FADE:
        mChannelPinState[channel] = TRANSITION_OFF;
        break;
      case BLINK:
        pwmValues[channel] = 0;
        mChannelPinState[channel] = OFF;
        break;
      case NEON:
        pwmValues[channel] = 0;
        mMode[channel] = NEON; // gets set to "FADE" during neon-startup
        mChannelPinState[channel] = OFF;
        break;
    }
  }
}

void OutputController::fade(uint8_t channel, uint16_t* nextEvent) {

  if (mChannelPinState[channel] == TRANSITION_ON || mChannelPinState[channel] == TRANSITION_OFF) {
    int8_t dir = (mChannelPinState[channel] == TRANSITION_ON) ? +1 : -1;
    uint8_t target = (dir > 0) ? mDimmValue[channel] : 0;
    State nextState = (dir > 0) ? ON : OFF;

    
    cli();
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    uint16_t overflowCounter = rtcOverflowCounter;
    __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
    sei();

    if(overflowCounter == *nextEvent) {
      *nextEvent += mFadeSpeed[channel];
      pwmValues[channel] += dir;
      if (pwmValues[channel] == target){
        mChannelPinState[channel] = nextState;
      }
    }
  }
}

void OutputController::blink(uint8_t channel, uint16_t* nextEvent) {
  bool isOn = (bool)pwmValues[channel];
  uint8_t waitTime = isOn ? mBlinkOffTime[channel] : mBlinkOnTime[channel];
  uint8_t nextValue = isOn ? 0 : mDimmValue[channel]; 

  cli();
  __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  uint16_t overflowCounter = rtcOverflowCounter;
  __asm__ __volatile__ ("" ::: "memory");  // Verhindert Optimierung
  sei();

  if(overflowCounter == *nextEvent) {
    *nextEvent += 10*waitTime;
    pwmValues[channel] = nextValue;
  }
}

enum TubeState{FLICKER, RAMP_UP};
void OutputController::neon(uint8_t channel, uint16_t* nextEvent) {
  // static uint8_t sCounter = 0;

  // if (mChannelPinState[channel] == TRANSITION_ON) {
  //   if(rtcOverflowCounter == *nextEvent) {
  //     if(sCounter++ >= ((lfsr16_next() % 5) + 6)) {
  //       pwmValues[channel] = mDimmValue[channel]/2;
  //       mMultiFunctionValue[channel] = 30; // langsames faden
  //       mMode[channel] = FADE;
  //     }
  //   }
  // }


  // static TubeState tubeState = FLICKER;
  // static uint8_t sCounter = 0;
  // uint8_t oldPwmValue[NUM_CHANNELS];
  // oldPwmValue[channel] = pwmValues[channel];

  // if (mChannelPinState[channel] == TRANSITION_ON) {
  //   if(rtcOverflowCounter == *nextEvent) {
  //     if(tubeState == FLICKER) {      
  //       if(sCounter++ > ((lfsr16_next() % 5) + 6)) {  // random(6,11)
  //         pwmValues[channel] = mDimmValue[channel];
  //         *nextEvent += ((lfsr16_next() % 200) + 100);  // random(100,300)
  //         tubeState = RAMP_UP;
  //       }
  //       else if(oldPwmValue[channel] == 0) {
  //         pwmValues[channel] = mDimmValue[channel];
  //         *nextEvent += ((lfsr16_next() % 70) + 30); // random(30,100)
  //       }
  //       else{
  //         pwmValues[channel] = 0;
  //         *nextEvent += ((lfsr16_next() % 150) + 50); // random(50,200)
  //       }        
  //     }
  //     else if(tubeState == RAMP_UP) {
  //       pwmValues[channel] = mDimmValue[channel]/2;
  //       mMultiFunctionValue[channel] = 30; // langsames faden
  //       mMode[channel] = FADE;
  //     }
  //   }
  // }
}

ISR(TCA0_CMP0_vect) {
  static uint8_t sPwmCounter = 0;

  for (uint8_t channel=0; channel<NUM_CHANNELS; channel++) {
    if (sPwmCounter < OutputController::pwmValues[channel]) {
      PORTA.OUTSET = OutputController::mChannelPin_bm[channel];  // High
    } else {
      PORTA.OUTCLR = OutputController::mChannelPin_bm[channel];  // Low
    }
  }
  sPwmCounter++;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; // Interrupt-Flag löschen
}

ISR(RTC_CNT_vect) {
  rtcOverflowCounter++;
  RTC.INTFLAGS = RTC_OVF_bm; // Flag löschen
}