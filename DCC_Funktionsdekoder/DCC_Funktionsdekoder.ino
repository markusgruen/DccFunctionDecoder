#include "DCCDecoder.h"

DCCDecoder dcc;

void setup() {
    dcc.begin();
}

void loop() {
    dcc.run();
}
