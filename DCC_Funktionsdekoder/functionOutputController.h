#ifndef FUNCTIONOUTPUTCONTROLLER_H
#define FUNCTIONOUTPUTCONTROLLER_H

#include "DccPacketHandler.h"  // for "Direction"

#define bit_switched_on(oldVar, newVar, bit) ((~oldVar & newVar) & (1<<bit))
#define bit_switched_off(oldVar, newVar, bit) ((oldVar & ~newVar) & (1<<bit))

class FunctionOutputController {
  public:
    FunctionOutputController();
    void begin();
    void readCVs();
    void update(Direction direction, uint8_t speed, uint32_t functions);

  private:
    uint32_t mFunctions;
   uint32_t mFunctionMap[4];

};




#endif