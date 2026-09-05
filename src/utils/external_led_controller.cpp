#include "external_led_controller.h"

#include <Arduino.h>

namespace external_led_controller {
namespace {

constexpr std::uint8_t PWM_CHANNEL = 0;
constexpr std::uint32_t PWM_FREQUENCY_HZ = 5000;
constexpr std::uint8_t PWM_RESOLUTION_BITS = 8;
constexpr unsigned long FRAME_INTERVAL_MS = 10;
constexpr unsigned long MIN_PERIOD_MS = 100;
constexpr unsigned long MAX_PERIOD_MS = 10000;

std::uint8_t ledPin = 4;
std::uint8_t ledBrightness = 128;
Effect currentEffect = Effect::Off;
unsigned long effectPeriodMs = 1000;
unsigned long effectStartedAt = 0;
unsigned long lastFrameAt = 0;

void writeBrightness(std::uint8_t value) {
    ledcWrite(PWM_CHANNEL, value);
}

void renderEffect() {
    const unsigned long elapsed = millis() - effectStartedAt;

    switch (currentEffect) {
        case Effect::Solid:
            writeBrightness(ledBrightness);
            break;

        case Effect::Blink: {
            const unsigned long halfPeriod = max(1UL, effectPeriodMs / 2);
            const bool visible = (elapsed / halfPeriod) % 2 == 0;
            writeBrightness(visible ? ledBrightness : 0);
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
            writeBrightness(static_cast<std::uint8_t>(
                (static_cast<std::uint16_t>(ledBrightness) * intensity) / 255
            ));
            break;
        }

        case Effect::Off:
            writeBrightness(0);
            break;
    }
}

}  // namespace

void begin(std::uint8_t pin) {
    ledPin = pin;
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS);
    ledcAttachPin(ledPin, PWM_CHANNEL);
    effectStartedAt = millis();
    renderEffect();
}

void update() {
    if (millis() - lastFrameAt >= FRAME_INTERVAL_MS) {
        lastFrameAt = millis();
        renderEffect();
    }
}

void setEffect(Effect newEffect) {
    currentEffect = newEffect;
    effectStartedAt = millis();
    renderEffect();
}

void setBrightness(std::uint8_t newBrightness) {
    ledBrightness = newBrightness;
    renderEffect();
}

void setPeriod(unsigned long periodMs) {
    effectPeriodMs = constrain(periodMs, MIN_PERIOD_MS, MAX_PERIOD_MS);
    effectStartedAt = millis();
    renderEffect();
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
        case Effect::Off:
            return "off";
    }

    return "unknown";
}

std::uint8_t brightness() {
    return ledBrightness;
}

unsigned long period() {
    return effectPeriodMs;
}

}  // namespace external_led_controller
