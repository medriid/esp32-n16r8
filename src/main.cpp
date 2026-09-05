#include <Arduino.h>

#include "utils/external_led_controller.h"
#include "utils/rgb_controller.h"
#include "utils/serial_command_controller.h"
#include "utils/web_led_controller.h"

void setup() {
    rgb_controller::begin(48);
    external_led_controller::begin(4);
    serial_command_controller::begin();
    web_led_controller::begin();
}

void loop() {
    rgb_controller::update();
    external_led_controller::update();
    serial_command_controller::update();
    web_led_controller::update();
    delay(2);
}
