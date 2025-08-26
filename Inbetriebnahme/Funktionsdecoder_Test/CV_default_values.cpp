#include "CV_default_values.h"
#include "CV_table.h"

const CVDefaults defaultCVs[] = {
  {SHORTADDRESS, 3},     // CV1 = 3 (z. B. Lok-Adresse)
 // {CONFIGBYTE,   0},   // CV29 = 0 (Default von 0 passt hier)

  {ch1FunctionMap0, 0b00000010},
  {ch2FunctionMap0, 0b00000100},
  {ch3FunctionMap0, 0b00001000},
  {ch4FunctionMap0, 0b00010000},

  {ch1DimmValue, 255},
  {ch2DimmValue, 255},
  {ch3DimmValue, 255},
  {ch4DimmValue, 255},

  {ch1BlinkOnTime, 10},
  {ch2BlinkOnTime, 10},
  {ch3BlinkOnTime, 10},
  {ch4BlinkOnTime, 10},

  {ch1BlinkOffTime, 50},
  {ch2BlinkOffTime, 50},
  {ch3BlinkOffTime, 50},
  {ch4BlinkOffTime, 50},
};

const uint8_t numDefaultCVs = sizeof(defaultCVs) / sizeof(CVDefaults);