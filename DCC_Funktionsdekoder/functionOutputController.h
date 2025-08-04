#ifndef FUNCTIONOUTPUTCONTROLLER_H
#define FUNCTIONOUTPUTCONTROLLER_H

#include "DccPacketHandler.h"  // for "Direction"


#define NUM_CHANNELS 4

#define PWM_MAX 0xFF

enum State{OFF, TRANSITION_ON, ON, TRANSITION_OFF};
enum Mode{ONOFF, FADE, NEON, BLINK, DOUBLE_BLINK, GYRA, MARS};

class FunctionOutputController {
  public:
    FunctionOutputController();
    static void begin(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4);
    void readCVs();
    void update(Direction direction, uint8_t speed, uint32_t functions);

    static uint8_t mChannelPin_bm[NUM_CHANNELS];
    static volatile uint8_t pwmValues[NUM_CHANNELS];

  private:
    void switchOn(uint8_t channel);
    void switchOff(uint8_t channel);
    bool forwardOnly(uint8_t channel) const {return bit_is_set(mFunctionMap[channel], 29);}
    bool reverseOnly(uint8_t channel) const {return bit_is_set(mFunctionMap[channel], 30);}

    bool isChannelOn(uint32_t functions, uint8_t channel) {
      return functions & mFunctionMap[channel]; }
    bool directionMatchesConfig(uint8_t channel, Direction direction) {
      if(forwardOnly(channel)) return direction == FORWARD;
      if(reverseOnly(channel)) return direction == REVERSE;
      return true; }
    bool IsSpeedBelowLimit(uint8_t channel, uint8_t speed) {
      return speed <= mSpeedThreshold[channel]; }

    

    uint32_t mFunctions;
    uint32_t mFunctionMap[NUM_CHANNELS];
    uint8_t mSpeedThreshold[NUM_CHANNELS];
    uint8_t mDimmValue[NUM_CHANNELS];
    Mode mMode[NUM_CHANNELS];
    Direction mDirection = FORWARD;

    State mChannelPinState[4]; 

    
};




#endif