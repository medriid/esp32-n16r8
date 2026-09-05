#pragma once

#include <cstdint>

namespace external_led_controller {

enum class Effect : std::uint8_t {
    Solid,
    Blink,
    Pulse,
    Off,
};

void begin(std::uint8_t pin);
void update();

void setEffect(Effect effect);
void setBrightness(std::uint8_t brightness);
void setPeriod(unsigned long periodMs);

Effect effect();
const char* effectName();
std::uint8_t brightness();
unsigned long period();

}  // namespace external_led_controller
