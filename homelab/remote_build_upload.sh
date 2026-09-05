#!/usr/bin/env bash

set -Eeuo pipefail

project_dir="${1:?project directory required}"
pio_binary="${2:?PlatformIO path required}"
pio_environment="${3:?PlatformIO environment required}"
upload_port="${4:?serial port required}"
compose_file="$project_dir/homelab/compose.yaml"
bridge_was_running=0

if [[ -f "$compose_file" ]] && \
   docker compose -f "$compose_file" ps --status running -q esp32-bridge | grep -q .; then
    bridge_was_running=1
    printf 'Pausing the ESP32 USB bridge for the firmware upload.\n'
    docker compose -f "$compose_file" stop -t 10 esp32-bridge
fi

restart_bridge() {
    if [[ "$bridge_was_running" -eq 1 ]]; then
        printf 'Restarting the ESP32 USB bridge.\n'
        docker compose -f "$compose_file" up -d --no-deps esp32-bridge
    fi
}
trap restart_bridge EXIT

test -x "$pio_binary"
test -e "$upload_port"
cd "$project_dir"
"$pio_binary" run -e "$pio_environment" -t upload --upload-port "$upload_port"

