#include "rgb_controller.h"

#include <Arduino.h>

namespace rgb_controller {
namespace {

constexpr unsigned long FRAME_INTERVAL_MS = 20;
constexpr unsigned long MIN_PERIOD_MS = 100;
constexpr unsigned long MAX_PERIOD_MS = 10000;

std::uint8_t rgbPin = 48;
std::uint8_t colorRed = 255;
std::uint8_t colorGreen = 0;
std::uint8_t colorBlue = 85;
std::uint8_t colorBrightness = 64;

Effect currentEffect = Effect::Solid;
unsigned long effectPeriodMs = 1000;
unsigned long effectStartedAt = 0;
unsigned long lastFrameAt = 0;

std::uint8_t scaleChannel(std::uint8_t channel, std::uint8_t intensity) {
    const std::uint32_t scaled =
        static_cast<std::uint32_t>(channel) * colorBrightness * intensity;
    return static_cast<std::uint8_t>(scaled / (255UL * 255UL));
}

void writeColor(
    std::uint8_t redValue,
    std::uint8_t greenValue,
    std::uint8_t blueValue,
    std::uint8_t intensity = 255
) {
    neopixelWrite(
        rgbPin,
        scaleChannel(redValue, intensity),
        scaleChannel(greenValue, intensity),
        scaleChannel(blueValue, intensity)
    );
}

void writeOff() {
    neopixelWrite(rgbPin, 0, 0, 0);
}

void rainbowColor(
    std::uint8_t position,
    std::uint8_t& redValue,
    std::uint8_t& greenValue,
    std::uint8_t& blueValue
) {
    if (position < 85) {
        redValue = 255 - position * 3;
        greenValue = position * 3;
        blueValue = 0;
        return;
    }

    if (position < 170) {
        position -= 85;
        redValue = 0;
        greenValue = 255 - position * 3;
        blueValue = position * 3;
        return;
    }

    position -= 170;
    redValue = position * 3;
    greenValue = 0;
    blueValue = 255 - position * 3;
}

void renderEffect() {
    const unsigned long elapsed = millis() - effectStartedAt;

    switch (currentEffect) {
        case Effect::Solid:
            writeColor(colorRed, colorGreen, colorBlue);
            break;

        case Effect::Blink: {
            const unsigned long halfPeriod = max(1UL, effectPeriodMs / 2);
            const bool visible = (elapsed / halfPeriod) % 2 == 0;
            if (visible) {
                writeColor(colorRed, colorGreen, colorBlue);
            } else {
                writeOff();
            }
            break;
        }

        case Effect::Pulse: {
            const unsigned long phase = elapsed % effectPeriodMs;
            const std::uint16_t triangle = static_cast<std::uint16_t>(
                (phase * 510UL) / effectPeriodMs
            );
            const std::uint8_t intensity = static_cast<std::uint8_t>(
                triangle <= 255 ? triangle : 510 - triangle
            );
            writeColor(colorRed, colorGreen, colorBlue, intensity);
            break;
        }

        case Effect::Rainbow: {
            const std::uint8_t position = static_cast<std::uint8_t>(
                ((elapsed % effectPeriodMs) * 256UL) / effectPeriodMs
            );
            std::uint8_t rainbowRed;
            std::uint8_t rainbowGreen;
            std::uint8_t rainbowBlue;
            rainbowColor(position, rainbowRed, rainbowGreen, rainbowBlue);
            writeColor(rainbowRed, rainbowGreen, rainbowBlue);
            break;
        }

        case Effect::Off:
            writeOff();
            break;
    }
}

}  // namespace

void begin(std::uint8_t pin) {
    rgbPin = pin;
    effectStartedAt = millis();
    renderEffect();
}

void update() {
    if (millis() - lastFrameAt >= FRAME_INTERVAL_MS) {
        lastFrameAt = millis();
        renderEffect();
    }
}

void setColor(std::uint8_t newRed, std::uint8_t newGreen, std::uint8_t newBlue) {
    colorRed = newRed;
    colorGreen = newGreen;
    colorBlue = newBlue;

    setEffect(Effect::Solid);
}

void setEffect(Effect newEffect) {
    currentEffect = newEffect;
    effectStartedAt = millis();
    renderEffect();
}

void setBrightness(std::uint8_t newBrightness) {
    colorBrightness = newBrightness;
    renderEffect();
}

void setPeriod(unsigned long periodMs) {
    effectPeriodMs = constrain(periodMs, MIN_PERIOD_MS, MAX_PERIOD_MS);
    effectStartedAt = millis();
    renderEffect();
}

void setBlinking(bool enabled) {
    setEffect(enabled ? Effect::Blink : Effect::Solid);
}

void turnOff() {
    setEffect(Effect::Off);
}

Effect effect() {
    return currentEffect;
}

const char* effectName() {
    switch (currentEffect) {
        case Effect::Solid:
            return "solid";
        case Effect::Blink:
            return "blink";
        case Effect::Pulse:
            return "pulse";
        case Effect::Rainbow:
            return "rainbow";
        case Effect::Off:
            return "off";
    }

    return "unknown";
}

std::uint8_t red() {
    return colorRed;
}

std::uint8_t green() {
    return colorGreen;
}

std::uint8_t blue() {
    return colorBlue;
}

std::uint8_t brightness() {
    return colorBrightness;
}

unsigned long period() {
    return effectPeriodMs;
}

}  // namespace rgb_controller
