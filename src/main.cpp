#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    const uint32_t flashBytes = ESP.getFlashChipSize();
    const uint32_t psramBytes = ESP.getPsramSize();
    const uint32_t freePsramBytes = ESP.getFreePsram();

    Serial.println();
    Serial.println("=== ESP32-S3 information ===");

    Serial.printf(
        "Flash:       %u bytes (%.2f MiB)\n",
        flashBytes,
        flashBytes / 1048576.0
    );

    Serial.printf(
        "PSRAM found: %s\n",
        psramFound() ? "yes" : "no"
    );

    Serial.printf(
        "PSRAM total: %u bytes (%.2f MiB usable)\n",
        psramBytes,
        psramBytes / 1048576.0
    );

    Serial.printf(
        "PSRAM free:  %u bytes (%.2f MiB)\n",
        freePsramBytes,
        freePsramBytes / 1048576.0
    );

    Serial.println("============================");
}

void loop() {
    delay(1000);
}