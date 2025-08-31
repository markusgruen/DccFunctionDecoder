#ifndef OUTPUTCONTROLLER_H
#define OUTPUTCONTROLLER_H

#include "DccPacketHandler.h"  // for "Direction"
#include "pinmap.h"

// Output operating modes
enum Mode { FADE, NEON, BLINK };

// Current PWM values for each channel (needs to be extern because of DccPacketHandler::confirmCVwrite();)
extern volatile uint8_t vPwmValue[NUM_CHANNELS];


namespace OutputController {
  
  // Target brightness values
  extern uint8_t dimmValue[NUM_CHANNELS];

  // Initialization and CV handling
  void begin();
  void readCVs();

  // Main execution loop
  void run(Direction direction, uint32_t functions);  

  // Output control state machines
  bool fade(uint8_t channel, bool fade_in);
  void blinkStateMachine(uint8_t channel, bool switch_on);
  void neonStateMachine(uint8_t channel, bool switch_on);
  bool wait(uint8_t channel, uint8_t waitTime, bool reset=false);

  // Utility functions for channel activation and config checks
  bool isChannelOn(uint32_t functions, uint8_t channel);
  bool directionMatchesConfig(uint8_t channel, Direction direction);
  bool IsSpeedBelowLimit(uint8_t channel, uint8_t speed);
  bool forwardOnly(uint8_t channel);
  bool reverseOnly(uint8_t channel);
};

uint16_t getPseudoRandomNumber();
uint8_t myRandomNumber(uint8_t min, uint8_t max);

#endif
