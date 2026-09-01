#include "rgb_controller.h"

#include <Arduino.h>
#include <cstdio>

namespace rgb_controller {
namespace {

constexpr unsigned long BLINK_INTERVAL_MS = 500;

std::uint8_t rgbPin = 48;
std::uint8_t red = 32;
std::uint8_t green = 0;
std::uint8_t blue = 0;

bool blinking = false;
bool ledOn = true;

unsigned long lastBlink = 0;
String input;

void updateLed() {
    if (ledOn) {
        neopixelWrite(rgbPin, red, green, blue);
    } else {
        neopixelWrite(rgbPin, 0, 0, 0);
    }
}

void showHelp() {
    Serial.println();
    Serial.println("RGB commands:");
    Serial.println("  R G B  - set a color, for example: 32 0 20");
    Serial.println("  blink  - blink the current color");
    Serial.println("  solid  - stop blinking");
    Serial.println("  off    - switch the LED off");
    Serial.println("  help   - show these commands");
    Serial.println();
    Serial.println("Start with values around 20-50; 255 is very bright.");
}

void handleCommand(String command) {
    command.trim();

    if (command.isEmpty()) {
        return;
    }

    String lower = command;
    lower.toLowerCase();

    if (lower == "blink") {
        setBlinking(true);
        Serial.println("Blinking");
        return;
    }

    if (lower == "solid") {
        setBlinking(false);
        Serial.println("Solid color");
        return;
    }

    if (lower == "off") {
        turnOff();
        Serial.println("LED off");
        return;
    }

    if (lower == "help") {
        showHelp();
        return;
    }

    int requestedRed;
    int requestedGreen;
    int requestedBlue;

    if (sscanf(
            command.c_str(),
            "%d %d %d",
            &requestedRed,
            &requestedGreen,
            &requestedBlue
        ) == 3) {
        setColor(
            static_cast<std::uint8_t>(constrain(requestedRed, 0, 255)),
            static_cast<std::uint8_t>(constrain(requestedGreen, 0, 255)),
            static_cast<std::uint8_t>(constrain(requestedBlue, 0, 255))
        );

        Serial.printf("Color set to R=%u G=%u B=%u\n", red, green, blue);
    } else {
        Serial.println("Unknown command. Type: help");
    }
}

}  // namespace

void begin(std::uint8_t pin) {
    rgbPin = pin;

    Serial.begin(115200);
    delay(1000);

    updateLed();
    Serial.println("ESP32-S3 RGB controller ready");
    showHelp();
}

void update() {
    while (Serial.available()) {
        const char character = Serial.read();

        if (character == '\n' || character == '\r') {
            if (!input.isEmpty()) {
                handleCommand(input);
                input = "";
            }
        } else {
            input += character;
        }
    }

    if (blinking && millis() - lastBlink >= BLINK_INTERVAL_MS) {
        lastBlink = millis();
        ledOn = !ledOn;
        updateLed();
    }
}

void setColor(std::uint8_t newRed, std::uint8_t newGreen, std::uint8_t newBlue) {
    red = newRed;
    green = newGreen;
    blue = newBlue;

    blinking = false;
    ledOn = true;
    updateLed();
}

void setBlinking(bool enabled) {
    blinking = enabled;
    ledOn = true;
    lastBlink = millis();
    updateLed();
}

void turnOff() {
    blinking = false;
    ledOn = false;
    updateLed();
}

}  // namespace rgb_controller
