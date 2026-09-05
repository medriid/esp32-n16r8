#include "web_led_controller.h"

#include "external_led_controller.h"
#include "rgb_controller.h"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace web_led_controller {
namespace {

constexpr char AP_SSID[] = "ESP32-RGB";
constexpr char AP_PASSWORD[] = "esp32rgb";
constexpr char HOSTNAME[] = "esp32-rgb";
constexpr unsigned long WIFI_TIMEOUT_MS = 12000;

WebServer server(80);

const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32 RGB</title>
  <style>
    :root{color-scheme:dark;font-family:system-ui,sans-serif;background:#101114;color:#f4f4f5}
    body{margin:0;min-height:100vh;display:grid;place-items:center;padding:20px;box-sizing:border-box}
    main{width:min(460px,100%);background:#1b1d22;border:1px solid #333740;border-radius:18px;padding:22px;box-sizing:border-box}
    h1{margin:0 0 4px;font-size:1.55rem}h2{margin:0 0 4px;font-size:1.15rem}p{margin:0 0 20px;color:#aeb2bc}
    section+section{margin-top:26px;padding-top:24px;border-top:1px solid #333740}
    label{display:block;margin:16px 0 7px;font-weight:600}
    input[type=color]{width:100%;height:72px;padding:4px;border:1px solid #424650;border-radius:12px;background:#101114}
    input[type=range]{width:100%;accent-color:#8b5cf6}
    .value{float:right;color:#c4b5fd;font-variant-numeric:tabular-nums}
    .effects{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin-top:18px}
    button{border:1px solid #424650;border-radius:11px;background:#292c33;color:#fff;padding:12px;font:inherit;font-weight:600;cursor:pointer}
    button:hover{background:#343841}button.active{background:#7c3aed;border-color:#8b5cf6}
    button.off{grid-column:1/-1;background:#3a2024;border-color:#653038}
    .status{margin-top:16px;color:#aeb2bc;font-size:.92rem}
  </style>
</head>
<body>
<main>
  <h1>ESP32-S3 LEDs</h1>
  <p>Local controls for both connected lights</p>

  <section>
  <h2>Onboard RGB LED</h2>
  <p>Colour LED built into the ESP32 board</p>

  <label for="color">Colour</label>
  <input id="color" type="color" value="#ff0055">

  <label for="brightness">Brightness <span class="value" id="brightnessValue">25%</span></label>
  <input id="brightness" type="range" min="1" max="100" value="25">

  <label for="speed">Animation time <span class="value" id="speedValue">1000 ms</span></label>
  <input id="speed" type="range" min="200" max="5000" step="100" value="1000">

  <div class="effects" aria-label="LED effects">
    <button data-effect="solid">Turn on / Solid</button>
    <button data-effect="blink">Blink</button>
    <button data-effect="pulse">Pulse</button>
    <button data-effect="rainbow">Rainbow</button>
    <button class="off" data-effect="off">Turn off</button>
  </div>

  <div class="status" id="status">Connecting…</div>
  </section>

  <section>
  <h2>External red LED — GPIO4</h2>
  <p>The loose red LED connected through the resistor</p>

  <label for="externalBrightness">Brightness <span class="value" id="externalBrightnessValue">50%</span></label>
  <input id="externalBrightness" type="range" min="1" max="100" value="50">

  <label for="externalSpeed">Animation time <span class="value" id="externalSpeedValue">1000 ms</span></label>
  <input id="externalSpeed" type="range" min="200" max="5000" step="100" value="1000">

  <div class="effects" aria-label="External LED effects">
    <button data-external-effect="solid">Turn on</button>
    <button data-external-effect="blink">Blink</button>
    <button data-external-effect="pulse">Pulse</button>
    <button class="off" data-external-effect="off">Turn off</button>
  </div>

  <div class="status" id="externalStatus">Connecting…</div>
  </section>
</main>
<script>
  const color = document.querySelector('#color');
  const brightness = document.querySelector('#brightness');
  const speed = document.querySelector('#speed');
  const brightnessValue = document.querySelector('#brightnessValue');
  const speedValue = document.querySelector('#speedValue');
  const status = document.querySelector('#status');
  const buttons = [...document.querySelectorAll('[data-effect]')];
  const externalBrightness = document.querySelector('#externalBrightness');
  const externalSpeed = document.querySelector('#externalSpeed');
  const externalBrightnessValue = document.querySelector('#externalBrightnessValue');
  const externalSpeedValue = document.querySelector('#externalSpeedValue');
  const externalStatus = document.querySelector('#externalStatus');
  const externalButtons = [...document.querySelectorAll('[data-external-effect]')];
  let selectedEffect = 'solid';
  let selectedExternalEffect = 'off';
  let timer;
  let externalTimer;

  function updateLabels() {
    brightnessValue.textContent = brightness.value + '%';
    speedValue.textContent = speed.value + ' ms';
  }

  function showEffect(effect) {
    selectedEffect = effect;
    buttons.forEach(button => button.classList.toggle('active', button.dataset.effect === effect));
  }

  function updateExternalLabels() {
    externalBrightnessValue.textContent = externalBrightness.value + '%';
    externalSpeedValue.textContent = externalSpeed.value + ' ms';
  }

  function showExternalEffect(effect) {
    selectedExternalEffect = effect;
    externalButtons.forEach(button => button.classList.toggle(
      'active',
      button.dataset.externalEffect === effect
    ));
  }

  async function apply(effect = selectedEffect) {
    showEffect(effect);
    const body = new URLSearchParams({
      color: color.value,
      brightness: brightness.value,
      speed: speed.value,
      effect
    });
    try {
      const response = await fetch('/api/control', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body
      });
      if (!response.ok) throw new Error('request failed');
      const state = await response.json();
      status.textContent = 'Active: ' + state.effect;
    } catch (_) {
      status.textContent = 'Could not reach the ESP32';
    }
  }

  function applySoon() {
    clearTimeout(timer);
    timer = setTimeout(() => apply(selectedEffect === 'off' ? 'solid' : selectedEffect), 80);
  }

  async function applyExternal(effect = selectedExternalEffect) {
    showExternalEffect(effect);
    const body = new URLSearchParams({
      brightness: externalBrightness.value,
      speed: externalSpeed.value,
      effect
    });
    try {
      const response = await fetch('/api/external/control', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body
      });
      if (!response.ok) throw new Error('request failed');
      const state = await response.json();
      externalStatus.textContent = 'Active: ' + state.effect;
    } catch (_) {
      externalStatus.textContent = 'Could not reach the ESP32';
    }
  }

  function applyExternalSoon() {
    clearTimeout(externalTimer);
    externalTimer = setTimeout(
      () => applyExternal(selectedExternalEffect === 'off' ? 'solid' : selectedExternalEffect),
      80
    );
  }

  buttons.forEach(button => button.addEventListener('click', () => apply(button.dataset.effect)));
  color.addEventListener('input', applySoon);
  brightness.addEventListener('input', () => { updateLabels(); applySoon(); });
  speed.addEventListener('input', () => { updateLabels(); applySoon(); });
  externalButtons.forEach(button => button.addEventListener(
    'click',
    () => applyExternal(button.dataset.externalEffect)
  ));
  externalBrightness.addEventListener('input', () => {
    updateExternalLabels();
    applyExternalSoon();
  });
  externalSpeed.addEventListener('input', () => {
    updateExternalLabels();
    applyExternalSoon();
  });

  async function loadState() {
    try {
      const state = await (await fetch('/api/state')).json();
      color.value = '#' + [state.red, state.green, state.blue]
        .map(value => value.toString(16).padStart(2, '0')).join('');
      brightness.value = Math.max(1, Math.round(state.brightness * 100 / 255));
      speed.value = state.period;
      showEffect(state.effect);
      updateLabels();
      status.textContent = 'Ready — active: ' + state.effect;
    } catch (_) {
      status.textContent = 'Could not reach the ESP32';
    }
  }

  async function loadExternalState() {
    try {
      const state = await (await fetch('/api/external/state')).json();
      externalBrightness.value = Math.max(1, Math.round(state.brightness * 100 / 255));
      externalSpeed.value = state.period;
      showExternalEffect(state.effect);
      updateExternalLabels();
      externalStatus.textContent = 'Ready — active: ' + state.effect;
    } catch (_) {
      externalStatus.textContent = 'Could not reach the ESP32';
    }
  }

  loadState();
  loadExternalState();
</script>
</body>
</html>
)HTML";

bool parseHexColor(
    const String& value,
    std::uint8_t& red,
    std::uint8_t& green,
    std::uint8_t& blue
) {
    if (value.length() != 7 || value[0] != '#') {
        return false;
    }

    char* end = nullptr;
    const unsigned long packed = strtoul(value.c_str() + 1, &end, 16);
    if (end == nullptr || *end != '\0') {
        return false;
    }

    red = static_cast<std::uint8_t>((packed >> 16) & 0xff);
    green = static_cast<std::uint8_t>((packed >> 8) & 0xff);
    blue = static_cast<std::uint8_t>(packed & 0xff);
    return true;
}

rgb_controller::Effect parseEffect(
    const String& value,
    rgb_controller::Effect fallback
) {
    if (value == "solid") {
        return rgb_controller::Effect::Solid;
    }
    if (value == "blink") {
        return rgb_controller::Effect::Blink;
    }
    if (value == "pulse") {
        return rgb_controller::Effect::Pulse;
    }
    if (value == "rainbow") {
        return rgb_controller::Effect::Rainbow;
    }
    if (value == "off") {
        return rgb_controller::Effect::Off;
    }
    return fallback;
}

void sendState() {
    char json[192];
    snprintf(
        json,
        sizeof(json),
        "{\"effect\":\"%s\",\"red\":%u,\"green\":%u,\"blue\":%u,"
        "\"brightness\":%u,\"period\":%lu}",
        rgb_controller::effectName(),
        rgb_controller::red(),
        rgb_controller::green(),
        rgb_controller::blue(),
        rgb_controller::brightness(),
        rgb_controller::period()
    );

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
}

void handleControl() {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    if (server.hasArg("color") &&
        parseHexColor(server.arg("color"), red, green, blue)) {
        rgb_controller::setColor(red, green, blue);
    }

    if (server.hasArg("brightness")) {
        const int percent = constrain(server.arg("brightness").toInt(), 0, 100);
        rgb_controller::setBrightness(static_cast<std::uint8_t>(percent * 255 / 100));
    }

    if (server.hasArg("speed")) {
        rgb_controller::setPeriod(static_cast<unsigned long>(
            constrain(server.arg("speed").toInt(), 100, 10000)
        ));
    }

    if (server.hasArg("effect")) {
        rgb_controller::setEffect(parseEffect(
            server.arg("effect"),
            rgb_controller::effect()
        ));
    }

    sendState();
}

external_led_controller::Effect parseExternalEffect(
    const String& value,
    external_led_controller::Effect fallback
) {
    if (value == "solid") {
        return external_led_controller::Effect::Solid;
    }
    if (value == "blink") {
        return external_led_controller::Effect::Blink;
    }
    if (value == "pulse") {
        return external_led_controller::Effect::Pulse;
    }
    if (value == "off") {
        return external_led_controller::Effect::Off;
    }
    return fallback;
}

void sendExternalState() {
    char json[128];
    snprintf(
        json,
        sizeof(json),
        "{\"effect\":\"%s\",\"brightness\":%u,\"period\":%lu}",
        external_led_controller::effectName(),
        external_led_controller::brightness(),
        external_led_controller::period()
    );

    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
}

void handleExternalControl() {
    if (server.hasArg("brightness")) {
        const int percent = constrain(server.arg("brightness").toInt(), 0, 100);
        external_led_controller::setBrightness(static_cast<std::uint8_t>(
            percent * 255 / 100
        ));
    }

    if (server.hasArg("speed")) {
        external_led_controller::setPeriod(static_cast<unsigned long>(
            constrain(server.arg("speed").toInt(), 100, 10000)
        ));
    }

    if (server.hasArg("effect")) {
        external_led_controller::setEffect(parseExternalEffect(
            server.arg("effect"),
            external_led_controller::effect()
        ));
    }

    sendExternalState();
}

bool connectToConfiguredWiFi() {
    if (strlen(WIFI_SSID) == 0) {
        return false;
    }

    Serial.printf("Connecting to Wi-Fi: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    return WiFi.status() == WL_CONNECTED;
}

void startAccessPoint() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
        Serial.println("Could not start the ESP32 Wi-Fi hotspot");
        return;
    }

    Serial.printf("Wi-Fi hotspot: %s\n", AP_SSID);
    Serial.printf("Hotspot password: %s\n", AP_PASSWORD);
    Serial.print("Open: http://");
    Serial.println(WiFi.softAPIP());
}

}  // namespace

void begin() {
    if (connectToConfiguredWiFi()) {
        Serial.print("Connected. Open: http://");
        Serial.println(WiFi.localIP());
    } else {
        startAccessPoint();
    }

    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("Local name: http://%s.local\n", HOSTNAME);
    }

    server.on("/", HTTP_GET, []() {
        server.sendHeader("Cache-Control", "no-store");
        server.send_P(200, "text/html", PAGE);
    });
    server.on("/api/state", HTTP_GET, sendState);
    server.on("/api/control", HTTP_POST, handleControl);
    server.on("/api/external/state", HTTP_GET, sendExternalState);
    server.on("/api/external/control", HTTP_POST, handleExternalControl);
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not found");
    });
    server.begin();
    Serial.println("RGB web controller ready");
}

void update() {
    server.handleClient();
}

}  // namespace web_led_controller
