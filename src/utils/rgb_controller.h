#pragma once

#include <cstdint>

namespace rgb_controller {

void begin(std::uint8_t pin);
void update();

void setColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue);
void setBlinking(bool enabled);
void turnOff();

}  // namespace rgb_controller
