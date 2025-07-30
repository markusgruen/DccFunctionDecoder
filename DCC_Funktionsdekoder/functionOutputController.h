#ifndef FUNCTIONOUTPUTCONTROLLER_H
#define FUNCTIONOUTPUTCONTROLLER_H

#include "DccPacketHandler.h"  // for "Direction"

#define bit_switched_on(oldVar, newVar, bit) ((~oldVar & newVar) & (1<<bit))
#define bit_switched_off(oldVar, newVar, bit) ((oldVar & ~newVar) & (1<<bit))

#define switched_on(oldVar, newVar) ( ~oldVar & newVar)
#define switched_off(oldVar, newVar) (oldVar & ~newVar)


enum State{OFF, TRANSITION_ON, ON, TRANSITION_OFF};

class FunctionOutputController {
  public:
    FunctionOutputController();
    void begin();
    void readCVs();
    void update(Direction direction, uint8_t speed, uint32_t functions);

  private:
    void switchOn(uint8_t channel);
    void switchOff(uint8_t channel);
    bool forwardOnly(uint8_t channel) const {
      return bit_is_set(mFunctionMap[channel], 29);
    }
    bool reverseOnly(uint8_t channel) const {
      return bit_is_set(mFunctionMap[channel], 30);
    }

    uint8_t mChannelPin_bm[4];
    
    uint32_t mFunctions;
    uint32_t mFunctionMap[4];

    Direction mDirection = FORWARD;

    State mChannelPinState[4]; 

};




#endif