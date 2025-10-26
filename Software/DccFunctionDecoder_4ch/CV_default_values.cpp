#include "CV_default_values.h"
#include "CV_table.h"

constexpr uint8_t LONG_ADDRESS_VALUE1 = 192 + DEFAULT_LONG_ADDRESS / 256;
constexpr uint8_t LONG_ADDRESS_VALUE2 = DEFAULT_LONG_ADDRESS % 256;


const CVDefaults defaultCVs[] = {
  {SHORTADDRESS, DEFAULT_SHORT_ADDRESS},
  {LONGADDRESS1, LONG_ADDRESS_VALUE1},
  {LONGADDRESS2, LONG_ADDRESS_VALUE2},

  {ch1FunctionMap0, 0b00000010},
  {ch2FunctionMap0, 0b00000100},
  {ch3FunctionMap0, 0b00001000},
  {ch4FunctionMap0, 0b00010000},

  {ch1DimmValue, 255},
  {ch2DimmValue, 255},
  {ch3DimmValue, 255},
  {ch4DimmValue, 255},

  {ch1BlinkOnTime, 80},
  {ch2BlinkOnTime, 80},
  {ch3BlinkOnTime, 80},
  {ch4BlinkOnTime, 80},

  {ch1BlinkOffTime, 150},
  {ch2BlinkOffTime, 150},
  {ch3BlinkOffTime, 150},
  {ch4BlinkOffTime, 150},
};

const uint8_t numDefaultCVs = sizeof(defaultCVs) / sizeof(CVDefaults);