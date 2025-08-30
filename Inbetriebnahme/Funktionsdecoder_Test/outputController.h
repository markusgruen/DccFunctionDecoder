#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "DccPacketHandler.h"  // for "Direction"
#include "pinmap.h"


#define PWM_MAX 0xFF

// enum State{OFF, SWITCH_ON, TRANSITION_ON, ON, SWITCH_OFF, TRANSITION_OFF};
enum Mode{FADE, NEON, BLINK};

extern volatile uint8_t vPwmValue[NUM_CHANNELS];

namespace OutputController {
  
  // uint32_t functionMap[NUM_CHANNELS];
  extern uint8_t dimmValue[NUM_CHANNELS];
  // uint8_t fadeSpeed[NUM_CHANNELS];
  // uint8_t blinkOnTime[NUM_CHANNELS];
  // uint8_t blinkOffTime[NUM_CHANNELS];
  // Mode mode[NUM_CHANNELS];
  // Direction direction = FORWARD;
  // State channelPinState[4];


  void begin();
  void readCVs();
  // void update(Direction direction, uint8_t speed, uint32_t functions);
  void run();  
  void update(Direction direction, uint32_t functions);

  bool fade(uint8_t channel, bool fade_in);
  void blinkStateMachine(uint8_t channel, bool switch_on);
  void neonStateMachine(uint8_t channel, bool switch_on);
  bool wait(uint8_t channel, uint8_t waitTime, bool reset=false);


  bool isChannelOn(uint32_t functions, uint8_t channel);
  bool directionMatchesConfig(uint8_t channel, Direction direction);
  bool IsSpeedBelowLimit(uint8_t channel, uint8_t speed);
  bool forwardOnly(uint8_t channel);
  bool reverseOnly(uint8_t channel);

};




#endif