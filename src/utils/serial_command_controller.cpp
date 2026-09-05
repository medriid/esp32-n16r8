#include "serial_command_controller.h"

#include "external_led_controller.h"
#include "rgb_controller.h"

#include <Arduino.h>
#include <cstdio>

namespace serial_command_controller {
namespace {

String input;

void printState() {
    Serial.printf(
        "{\"type\":\"state\",\"rgb\":{\"effect\":\"%s\","
        "\"red\":%u,\"green\":%u,\"blue\":%u,\"brightness\":%u,"
        "\"period\":%lu},\"external\":{\"effect\":\"%s\","
        "\"brightness\":%u,\"period\":%lu}}\n",
        rgb_controller::effectName(),
        rgb_controller::red(),
        rgb_controller::green(),
        rgb_controller::blue(),
        rgb_controller::brightness(),
        rgb_controller::period(),
        external_led_controller::effectName(),
        external_led_controller::brightness(),
        external_led_controller::period()
    );
}

void showHelp() {
    Serial.println();
    Serial.println("ESP32-S3 LED commands:");
    Serial.println("  rgb color R G B");
    Serial.println("  rgb effect solid|blink|pulse|rainbow|off");
    Serial.println("  rgb brightness 0..255");
    Serial.println("  rgb speed 100..10000");
    Serial.println("  external effect solid|blink|pulse|off");
    Serial.println("  external brightness 0..255");
    Serial.println("  external speed 100..10000");
    Serial.println("  state");
    Serial.println("  help");
    Serial.println();
    Serial.println("Legacy RGB commands still work: R G B, solid, blink, pulse,");
    Serial.println("rainbow, off, brightness N, and speed N.");
}

bool setRgbEffect(const String& name) {
    if (name == "solid") {
        rgb_controller::setEffect(rgb_controller::Effect::Solid);
    } else if (name == "blink") {
        rgb_controller::setEffect(rgb_controller::Effect::Blink);
    } else if (name == "pulse") {
        rgb_controller::setEffect(rgb_controller::Effect::Pulse);
    } else if (name == "rainbow") {
        rgb_controller::setEffect(rgb_controller::Effect::Rainbow);
    } else if (name == "off") {
        rgb_controller::setEffect(rgb_controller::Effect::Off);
    } else {
        return false;
    }
    return true;
}

bool setExternalEffect(const String& name) {
    if (name == "solid") {
        external_led_controller::setEffect(external_led_controller::Effect::Solid);
    } else if (name == "blink") {
        external_led_controller::setEffect(external_led_controller::Effect::Blink);
    } else if (name == "pulse") {
        external_led_controller::setEffect(external_led_controller::Effect::Pulse);
    } else if (name == "off") {
        external_led_controller::setEffect(external_led_controller::Effect::Off);
    } else {
        return false;
    }
    return true;
}

bool handleRgbCommand(const String& command) {
    int first;
    int second;
    int third;

    if (sscanf(command.c_str(), "color %d %d %d", &first, &second, &third) == 3) {
        rgb_controller::setColor(
            static_cast<std::uint8_t>(constrain(first, 0, 255)),
            static_cast<std::uint8_t>(constrain(second, 0, 255)),
            static_cast<std::uint8_t>(constrain(third, 0, 255))
        );
        return true;
    }

    char effectName[16];
    if (sscanf(command.c_str(), "effect %15s", effectName) == 1) {
        return setRgbEffect(String(effectName));
    }

    if (sscanf(command.c_str(), "brightness %d", &first) == 1) {
        rgb_controller::setBrightness(
            static_cast<std::uint8_t>(constrain(first, 0, 255))
        );
        return true;
    }

    if (sscanf(command.c_str(), "speed %d", &first) == 1) {
        rgb_controller::setPeriod(
            static_cast<unsigned long>(constrain(first, 100, 10000))
        );
        return true;
    }

    return false;
}

bool handleExternalCommand(const String& command) {
    int value;
    char effectName[16];

    if (sscanf(command.c_str(), "effect %15s", effectName) == 1) {
        return setExternalEffect(String(effectName));
    }

    if (sscanf(command.c_str(), "brightness %d", &value) == 1) {
        external_led_controller::setBrightness(
            static_cast<std::uint8_t>(constrain(value, 0, 255))
        );
        return true;
    }

    if (sscanf(command.c_str(), "speed %d", &value) == 1) {
        external_led_controller::setPeriod(
            static_cast<unsigned long>(constrain(value, 100, 10000))
        );
        return true;
    }

    return false;
}

bool handleLegacyRgbCommand(const String& command) {
    if (setRgbEffect(command)) {
        return true;
    }

    int first;
    int second;
    int third;

    if (sscanf(command.c_str(), "brightness %d", &first) == 1) {
        rgb_controller::setBrightness(
            static_cast<std::uint8_t>(constrain(first, 0, 255))
        );
        return true;
    }

    if (sscanf(command.c_str(), "speed %d", &first) == 1) {
        rgb_controller::setPeriod(
            static_cast<unsigned long>(constrain(first, 100, 10000))
        );
        return true;
    }

    if (sscanf(command.c_str(), "%d %d %d", &first, &second, &third) == 3) {
        rgb_controller::setColor(
            static_cast<std::uint8_t>(constrain(first, 0, 255)),
            static_cast<std::uint8_t>(constrain(second, 0, 255)),
            static_cast<std::uint8_t>(constrain(third, 0, 255))
        );
        return true;
    }

    return false;
}

void handleCommand(String command) {
    command.trim();
    command.toLowerCase();

    if (command.isEmpty()) {
        return;
    }

    if (command == "help") {
        showHelp();
        return;
    }

    if (command == "state") {
        printState();
        return;
    }

    bool handled = false;
    if (command.startsWith("rgb ")) {
        handled = handleRgbCommand(command.substring(4));
    } else if (command.startsWith("external ")) {
        handled = handleExternalCommand(command.substring(9));
    } else {
        handled = handleLegacyRgbCommand(command);
    }

    if (!handled) {
        Serial.println("Unknown command. Type: help");
        return;
    }

    printState();
}

}  // namespace

void begin() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-S3 LED controller ready");
    showHelp();
    printState();
}

void update() {
    while (Serial.available()) {
        const char character = Serial.read();
        if (character == '\n' || character == '\r') {
            if (!input.isEmpty()) {
                handleCommand(input);
                input = "";
            }
        } else if (input.length() < 127) {
            input += character;
        }
    }
}

}  // namespace serial_command_controller
