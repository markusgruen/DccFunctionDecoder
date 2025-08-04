#include "CV_default_values.h"

const CVDefaults defaultCVs[] = {
  {1, 3},     // CV1 = 3 (z. B. Lok-Adresse)
  {29, 6},    // CV29 = 6 (z. B. Konfigurationsregister)
};

const uint8_t numDefaultCVs = sizeof(defaultCVs) / sizeof(CVDefaults);