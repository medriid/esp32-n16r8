#include <Arduino.h>

#include "utils/rgb_controller.h"

void setup() {
    rgb_controller::begin(48);
}

void loop() {
    rgb_controller::update();
}
