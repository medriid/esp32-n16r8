# Homelab services for the ESP32-S3

The ESP32 stays connected to the Debian homelab by USB. The homelab runs the
control bridge, MQTT broker, and Home Assistant. The ESP32 does not need to join
the hostel Wi-Fi.

## Current topology

```text
Laptop/phone -- Tailscale --> Debian homelab -- USB --> ESP32-S3
                                  |
                                  +-- Home Assistant :8123
                                  +-- LED controller :8080
                                  +-- MQTT 127.0.0.1:1883 only
```

The homelab firewall permits the web services through Tailscale and blocks
unsolicited traffic from the hostel Wi-Fi interface. MQTT is additionally bound
to loopback and cannot be reached from either network.

## Private URLs

- Home Assistant: `http://mdebian-homelab:8123`
- ESP32 LED controller: `http://mdebian-homelab:8080`

The numeric fallback is `100.107.43.20` in place of `mdebian-homelab`.

## Protected domains

- `home.larpvit.me` -> Home Assistant
- `esp.larpvit.me` -> ESP32 LED controller

Both hostnames are published through the existing remotely managed
`craftih-homelab` Cloudflare Tunnel. Cloudflare redirects HTTP to HTTPS and the
existing approved-account Access policy protects both applications. Device
enrollment remains to be completed before changing the policy to require the
approved laptop or phone as well as an approved account.

## Containers

The versioned service definition is `homelab/compose.yaml`. Runtime state is
stored outside the source tree under `/home/log/services/esp32-home/` on the
homelab.

From the laptop:

```bash
ssh mdebian-homelab 'cd ~/esp32/ccode/homelab && docker compose ps'
ssh mdebian-homelab 'cd ~/esp32/ccode/homelab && docker compose logs -f esp32-bridge'
ssh mdebian-homelab 'cd ~/esp32/ccode/homelab && docker compose up -d --build'
```

## Firmware workflow

Edit the project normally on the laptop, then run:

```bash
pio run --target upload
```

The upload script synchronizes the source, builds on the homelab, temporarily
pauses the USB bridge, flashes the board using its stable device identity, and
restarts the bridge. Do not start a separate serial monitor while the bridge is
running because only one program can own the serial port at a time.

## Home Assistant and MQTT

The bridge publishes retained Home Assistant discovery data for two lights:

- `Desk ESP32-S3 Onboard RGB`
- `Desk ESP32-S3 External red LED`

After completing Home Assistant onboarding, add the MQTT integration using:

- Broker: `127.0.0.1`
- Port: `1883`
- Username/password: leave blank

The discovered device and both light entities should then appear automatically.
This anonymous MQTT listener is safe only because it is bound to loopback. Add
authentication before exposing MQTT to a future private hardware Wi-Fi network.
