#include "DccSignalDecoder.h"

DccSignalDecoder dcc;

void setup() {
    dcc.begin(PIN_PA2);
}

void loop() {
    dcc.run();
}
