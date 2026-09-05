#!/usr/bin/env bash

set -Eeuo pipefail

remote_host="${PIO_REMOTE_HOST:-log@mdebian-homelab}"
remote_port="${PIO_REMOTE_UPLOAD_PORT:-/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C38119516-if00}"

exec ssh -t "$remote_host" \
    "/home/log/.local/bin/pio device monitor --port '$remote_port' --baud 115200"
