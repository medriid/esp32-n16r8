# ESP32-S3 Home Assistant Device Plan

This is the living plan and inventory for the ESP32-S3 project. Update the
checkboxes and notes as the build changes. Items marked **optional** should not
be purchased unless that part of the project is actually chosen.

## Project goal

Build a local-first ESP32-S3 device that can:

- connect over Wi-Fi to services on the homelab;
- appear in Home Assistant and exchange commands and state;
- control its onboard RGB LED and simple external electronics;
- show Home Assistant information on a small display;
- later act as a Home Assistant voice satellite with a microphone and speaker;
- remain portable to another machine on the same LAN by changing its configured
  server hostname or IP address;
- optionally control motors as a separate learning project.

The ESP32 is a client/peripheral. Home Assistant, MQTT, and voice processing
belong on the homelab rather than on the ESP32 itself.

## Current confirmed setup

- [x] ESP32-S3 N16R8 board identified.
- [x] 16 MiB flash detected.
- [x] 8 MiB PSRAM detected and usable.
- [x] PlatformIO project builds and uploads.
- [x] Serial monitor works through `/dev/ttyACM0` at 115200 baud.
- [x] Onboard addressable RGB LED found on GPIO48.
- [x] Onboard RGB colour control tested successfully.
- [x] RGB controller moved into `src/utils/`.
- [x] Git repository/template created.
- [x] Shipping foam removed from the header pins.
- [x] Confirmed that this ESP32 board is too wide to expose usable terminal
  holes when centred on the MB-102 breadboard.
- [x] Implemented and compiled a local web controller for the onboard RGB LED.
- [x] Added solid, blink, pulse, rainbow, brightness and speed controls.
- [x] Added independent web controls for the external red LED on GPIO4.
- [x] Audited the homelab network and confirmed Tailscale access through the
  hostel network without exposing the ESP32 directly to it.
- [x] Connected the ESP32 permanently to the homelab over USB and recorded its
  stable serial-device identity.
- [x] Installed an isolated PlatformIO Core environment on the homelab.
- [x] Configured `pio run --target upload` on the development laptop to sync,
  build and flash through the homelab.
- [x] Added machine-readable USB serial control for both the onboard RGB LED
  and the external red LED.
- [x] Deployed a persistent USB-to-HTTP/MQTT bridge on the homelab.
- [x] Deployed a local-only Mosquitto broker on `127.0.0.1:1883`.
- [x] Deployed Home Assistant Container on the homelab.
- [x] Made remote firmware uploads automatically pause and restart the USB
  bridge so there is no serial-port conflict.
- [x] Finish the one-time Home Assistant owner-account onboarding.
- [ ] Add Home Assistant's MQTT integration and confirm both discovered lights.
- [x] Add `home.larpvit.me` and `esp.larpvit.me` to the existing Cloudflare
  Tunnel with automatic HTTPS and the existing approved-account Access policy.
- [ ] Enroll the laptop and phone in Cloudflare One, then require an approved
  device as well as an approved account for both homelab applications.

Current PlatformIO environments:

```ini
[env]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

monitor_speed = 115200

board_build.flash_mode = qio
board_build.arduino.memory_type = qio_opi
board_build.partitions = default_16MB.csv

board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216

build_flags =
    -DBOARD_HAS_PSRAM

[env:esp32-s3-n16r8]
upload_protocol = custom
upload_command = bash scripts/remote_upload.sh

[env:esp32-s3-n16r8-homelab]
upload_protocol = esptool
upload_port = /dev/serial/by-id/usb-1a86_USB_Single_Serial_5C38119516-if00
```

Useful commands:

```bash
pio run
pio run --target upload          # build/flash through the homelab
./scripts/remote_monitor.sh      # serial monitor through the homelab
pio run --target compiledb
```

Current homelab endpoints (available through Tailscale):

```text
Home Assistant:       http://mdebian-homelab:8123
ESP32 LED controller: http://mdebian-homelab:8080
MQTT broker:          127.0.0.1:1883 on the homelab only
```

Planned protected domain endpoints:

```text
Home Assistant:       https://home.larpvit.me
ESP32 LED controller: https://esp.larpvit.me
```

## Hardware already owned

| Item | Quantity | Purpose | Status/notes |
|---|---:|---|---|
| ESP32-S3 N16R8 development board | 1 | Main Wi-Fi/Bluetooth controller | Working; 16 MiB flash and 8 MiB PSRAM verified |
| MB-102 830-point solderless breadboard | 1 | Temporary circuits without soldering | Ready to use |
| Male-to-male Dupont jumpers | About 40 | Breadboard-to-breadboard connections | Owned |
| Male-to-female Dupont jumpers | About 40 | ESP32/module pins to breadboard | Owned |
| Female-to-female Dupont jumpers | About 40 | Connections between exposed male headers | Owned |
| 1/4 W metal-film resistor assortment | 60 | LEDs, pull-ups, pull-downs and experiments | Values must be checked before use |
| 5 mm red DIP LEDs | 10 | External LED experiments | Red only; brightness can vary, colour cannot |
| 1x40 2.54 mm breakaway pin header | 1 strip | Solder onto modules that arrive with empty holes | Do not use until a module needs it |
| DRV8833 dual motor-driver breakout | 1 | Two brushed DC motors or one bipolar stepper | Keep disconnected until motors and a suitable supply exist |
| Data-capable USB cable | At least 1 | Power, upload and serial monitoring | Working |
| Linux computer with VSCodium and PlatformIO | 1 | Development workstation | Working |
| Homelab | 1 | Home Assistant, MQTT and later voice services | Debian 13; Docker and Tailscale active; remote ESP32 USB upload configured |

## Hardware expected soon

| Item | Expected | First action |
|---|---|---|
| Small display/OLED | Tomorrow | Photograph the front, back, controller markings and pin labels before wiring |
| Microphone | In one or two weeks | Confirm it is a 3.3 V-compatible digital I2S microphone before connecting |
| Speaker | In one or two weeks | Confirm impedance/power rating and use it only through a proper amplifier |

## Recommended future purchases

### High value / recommended soon

- [ ] **Digital multimeter** with DC voltage, resistance and continuity modes.
  This is the most useful missing diagnostic tool.
- [ ] A few **momentary tactile push-buttons** for local controls and learning
  digital inputs.
- [ ] A small **10 kOhm potentiometer** for learning analog input, if desired.
- [ ] A stable **5 V USB power supply**, ideally 2 A or better, for operating the
  finished non-motor device without the computer. Buy only from a reputable
  source.

### Display-related: wait for the display first

- [ ] Nothing if the display arrives with its header already soldered.
- [ ] Soldering equipment only if its header is loose or the build is being made
  permanent. The owned 1x40 strip can supply the display header.

### Voice hardware: buy/confirm as one compatible set

- [ ] One 3.3 V-compatible **I2S digital microphone module**, such as an INMP441
  or another model supported by the chosen firmware.
- [ ] One **I2S DAC/amplifier module**, commonly a MAX98357A-compatible module.
- [ ] One **4 or 8 ohm speaker** whose wattage is suitable for the amplifier.
- [ ] Optional physical push-to-talk or mute button.
- [ ] Optional enclosure with proper microphone and speaker openings.

Do not connect a raw speaker directly to an ESP32 GPIO. The DRV8833 is a motor
driver, not a speaker amplifier.

### Motor branch: optional and unrelated to the voice assistant

- [ ] One or two brushed DC motors, or one bipolar stepper motor.
- [ ] A separate motor power supply matched to the chosen motor and within the
  driver module's allowed range.
- [ ] Power switch and suitable connectors.
- [ ] Extra decoupling/bulk capacitor if the particular breakout does not
  already provide enough.

Do not buy these unless a motor project is actually planned. Motors must not be
powered from the ESP32 3.3 V pin.

### Permanent-build and soldering supplies: later

- [ ] Temperature-controlled soldering iron with stand.
- [ ] Electronics solder and flux.
- [ ] Brass tip cleaner or damp sponge.
- [ ] Side cutters and wire stripper.
- [ ] Helping hands or a PCB holder.
- [ ] Heat-shrink tubing.
- [ ] Perfboard or a custom PCB.
- [ ] Enclosure, spacers, screws and cable strain relief.
- [ ] Fume extraction or very good ventilation.

The breadboard stage requires no soldering.

## Breadboard mounting procedure

The white foam around/beneath the ESP32 in the current photo appears to be
shipping protection for the header pins. It must not remain between the board
and breadboard during use.

1. Unplug both USB ports and remove all power.
2. If the foam is loose packaging, hold the PCB by its edges and pull the foam
   off the header pins gradually and evenly.
3. Do not pull on the USB connectors, buttons, antenna area or individual pins.
4. If the foam is actually glued to the PCB rather than merely pushed over the
   pins, stop and inspect the underside before peeling it.
5. Check that all header pins are straight.
6. Place the ESP32 so one header row enters the terminal strip on each side of
   the breadboard's centre trench.
7. Let the USB end overhang the end of the breadboard so cables do not lever the
   board upward.
8. Confirm every pin is aligned with a hole, then press both long sides down
   evenly. Do not force it.
9. It is normal for the PCB to sit above the breadboard on its plastic header
   spacers. The underside of the PCB should not rest directly on the breadboard.

This board has now been confirmed to be too wide to leave accessible holes
beside either header row. Do not improvise by using the power rails as signal
rows. Keep the ESP32 beside the breadboard and connect it using male-to-female
jumpers. A second breadboard or a suitable ESP32 breakout/expansion board is a
possible later convenience, but is not required for the first experiments.

## Breadboard connection rules

- Each numbered group of five holes on one side of the centre trench is
  internally connected.
- The five holes across the centre trench are a separate group.
- Side power rails run lengthwise, but they may be split around the midpoint.
- Rail colours are labels only; they do not create voltage by themselves.
- Never connect both 3.3 V and 5 V to the same rail.
- Wire or change components only with USB/external power disconnected.

## Provisional pin plan

This plan can change after the display and audio hardware are identified.

| Function | ESP32 pin | Status |
|---|---:|---|
| Onboard addressable RGB LED | GPIO48 | Confirmed and working |
| First external red LED | GPIO4 | Proposed for the breadboard lesson |
| I2C display SDA | GPIO8 | Provisional; confirm after display arrives |
| I2C display SCL | GPIO9 | Provisional; confirm after display arrives |
| I2S microphone | TBD | Assign after exact microphone is known |
| I2S speaker amplifier | TBD | Assign after exact amplifier is known |
| Local buttons | TBD | Assign after display/audio pins are reserved |
| DRV8833 inputs | TBD | Assign only if the motor branch begins |

## Build roadmap

### Phase 1: breadboard and external LED

- [x] Remove the loose shipping foam with power disconnected.
- [x] Remove the ESP32 from the MB-102 and place it safely beside the breadboard;
  this board is too wide to expose its connected terminal holes.
- [x] Identify a suitable LED resistor from the assortment.
- [x] Wire GPIO4 -> resistor -> red LED -> GND.
- [x] Upload and verify a basic blink program.
- [ ] Test different blink timings.
- [ ] Learn why the resistor is required and how breadboard rows connect.
- [ ] Replace blocking `delay()` logic with a `millis()`-based update.
- [ ] Add on/off and brightness control as a small utility module.

### Phase 2: display

- [ ] Record the display model, controller, voltage and pin labels.
- [ ] Determine whether it uses I2C or SPI; do not assume from appearance.
- [ ] Solder its header only if it arrives unsoldered.
- [ ] Power it from 3.3 V unless its verified documentation requires otherwise.
- [ ] For an I2C display, run an address scanner before installing display code.
- [ ] Display `Hello`, uptime and Wi-Fi state.
- [ ] Create a reusable display utility under `src/utils/`.
- [ ] Design a small Home Assistant status screen.

### Phase 3: homelab and MQTT foundation

- [x] Add an ESP32-hosted local RGB control page with a temporary Wi-Fi hotspot
  fallback.
- [x] Audit the current homelab network, routes and firewall.
- [x] Confirm the open hostel Wi-Fi remains the internet uplink while Tailscale
  provides private administration access.
- [x] Configure Tailscale SSH for the homelab.
- [x] Identify the ESP32 as
  `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C38119516-if00`.
- [x] Give the `log` account serial-port access through the `dialout` group.
- [x] Install PlatformIO Core 6.1.19 in the `log` account's private virtual
  environment.
- [x] Add guarded project synchronization and remote USB upload scripts.
- [x] Upload and test the RGB control page on the physical ESP32.
- [x] Upload and test the external GPIO4 LED controls.
- [x] Document Home Assistant Container running under Docker on Debian 13.
- [x] Install and confirm a loopback-only Mosquitto MQTT broker on the homelab.
- [ ] Create a dedicated MQTT account for the ESP32.
- [ ] Give the homelab a reserved LAN IP and/or stable local DNS name.
- [ ] Do not use `localhost` in ESP32 configuration: on the ESP32, `localhost`
  means the ESP32 itself.
- [ ] Keep Wi-Fi and MQTT credentials in an ignored secrets file.
- [ ] Connect the ESP32 to Wi-Fi with retry/reconnect handling.
- [ ] Publish online/offline availability, uptime and signal strength.
- [ ] Subscribe to a command topic and control the onboard RGB LED.
- [ ] Publish the resulting LED state back to MQTT.
- [ ] Add Home Assistant MQTT discovery so the device appears automatically.
- [ ] Confirm control works entirely inside the LAN with the internet disabled.

Possible initial topics:

```text
home/desk-esp32/status
home/desk-esp32/rgb/set
home/desk-esp32/rgb/state
home/desk-esp32/display/set
```

Topic names are provisional. Authentication is required, and the MQTT broker
must not be exposed directly to the public internet.

### Phase 4: useful Home Assistant desk device

- [ ] Show time, room state and selected Home Assistant entities on the display.
- [ ] Add physical button input for a chosen Home Assistant action.
- [ ] Show Wi-Fi/MQTT connection status without filling the serial log.
- [ ] Add graceful recovery after Wi-Fi, broker or homelab restarts.
- [ ] Add over-the-air firmware updates only after basic recovery is reliable.
- [ ] Decide whether the external LED remains part of the final device.

### Phase 5: voice assistant

- [ ] Choose the firmware direction:
  - **Recommended first:** ESPHome voice assistant for quicker Home Assistant
    integration.
  - **Learning-heavy alternative:** custom PlatformIO/C++ audio and protocol
    implementation.
- [ ] Keep the working PlatformIO project even if the board is temporarily
  flashed with ESPHome; only one firmware can run at a time.
- [ ] Test microphone input alone.
- [ ] Test amplifier and speaker output alone at low volume.
- [ ] Add push-to-talk before attempting wake-word detection.
- [ ] Connect to the Home Assistant Assist pipeline.
- [ ] Configure local speech-to-text, text-to-speech and optional wake-word
  processing on the homelab.
- [ ] Measure stability and memory use before enabling unrelated Bluetooth work.
- [ ] Add hardware or software mute indication and a privacy-conscious design.

### Phase 6: reliability and permanent assembly

- [ ] Measure actual current consumption.
- [ ] Choose a suitable dedicated power supply.
- [ ] Add startup, disconnect and error handling.
- [ ] Run an extended stability test.
- [ ] Draw and save the final wiring diagram.
- [ ] Move from loose breadboard wiring to perfboard/custom PCB only after the
  design is stable.
- [ ] Add an enclosure, ventilation, strain relief and access to reset/boot.
- [ ] Label cables and keep a final bill of materials.

### Phase 7: optional motor project

- [ ] Photograph both sides and every label on the DRV8833 breakout.
- [ ] Select a motor and matching separate power supply.
- [ ] Confirm the breakout's `nSLEEP`, input, output and current-limit wiring.
- [ ] Connect ESP32 ground and motor-supply ground together.
- [ ] Test one motor at low duty cycle with no mechanical load.
- [ ] Add direction and speed control.
- [ ] Keep motor wiring and electrical noise away from microphone/audio wiring.

## Immediate next circuit

With USB unplugged:

```text
GPIO4 ---- 330 ohm resistor ---- LED long leg
                                    LED short leg ---- GND
```

Then upload this from a separate lab copy of the template rather than replacing
the clean template permanently:

```cpp
#include <Arduino.h>

constexpr uint8_t LED_PIN = 4;

void setup() {
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
}
```

The serial monitor will remain blank because this program does not print. The
external LED itself is the output.

## Non-negotiable safety rules

- Treat every ESP32 GPIO as 3.3 V logic; never apply 5 V to a GPIO.
- Always use a series resistor with a normal LED.
- Disconnect power before moving breadboard wires or components.
- Check the complete pin label twice before connecting power.
- Never connect a motor, raw speaker, relay coil or other heavy load directly to
  a GPIO.
- Use a proper audio amplifier for a speaker.
- Use a proper motor driver and separate matched motor supply for motors.
- Join grounds when two powered circuits exchange signals, unless a design
  intentionally provides isolation.
- Do not power the same board from unrelated supplies simultaneously unless the
  power arrangement has been explicitly verified.
- Keep powered PCBs away from metal surfaces, loose wire clippings and liquids.
- Stop immediately if anything becomes hot, smells unusual or repeatedly resets.

## Information still needed

- [ ] Clear front/back/pin-label photographs of the display when it arrives.
- [x] Home Assistant Container under Docker on Debian 13.
- [x] Mosquitto runs in Docker and listens only on homelab loopback.
- [ ] Exact microphone, amplifier and speaker models before purchase or wiring.
- [ ] Clear front/back/pin-label photographs of the DRV8833 before motor work.
- [ ] Whether a digital multimeter and soldering iron are already available.

## Reference links

- [ESP32-S3 DevKitC-1 user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html)
- [Home Assistant MQTT integration](https://www.home-assistant.io/integrations/mqtt/)
- [ESPHome SSD1306 display documentation](https://esphome.io/components/display/ssd1306/)
- [ESPHome voice assistant documentation](https://esphome.io/components/voice_assistant/)
- [Texas Instruments DRV8833 datasheet](https://www.ti.com/lit/ds/symlink/drv8833.pdf)
