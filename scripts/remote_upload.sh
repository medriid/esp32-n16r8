#!/usr/bin/env bash

set -Eeuo pipefail

project_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
remote_host="${PIO_REMOTE_HOST:-log@mdebian-homelab}"
remote_dir="${PIO_REMOTE_DIR:-/home/log/esp32/ccode}"
remote_port="${PIO_REMOTE_UPLOAD_PORT:-/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C38119516-if00}"
remote_pio="/home/log/.local/bin/pio"
remote_env="esp32-s3-n16r8-homelab"
ssh_options=(-o BatchMode=yes -o ConnectTimeout=15)

for command_name in ssh rsync; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        printf 'Remote upload requires %s on the laptop.\n' "$command_name" >&2
        exit 1
    fi
done

if [[ ! "$remote_host" =~ ^[A-Za-z0-9_.@-]+$ ]]; then
    printf 'Unsafe PIO_REMOTE_HOST value: %s\n' "$remote_host" >&2
    exit 1
fi

if [[ "$remote_dir" != /home/log/esp32/* || "$remote_dir" == *..* ]]; then
    printf 'Refusing unsafe remote project path: %s\n' "$remote_dir" >&2
    exit 1
fi

if [[ "$remote_port" != /dev/serial/by-id/* && "$remote_port" != /dev/ttyACM* ]]; then
    printf 'Refusing unexpected remote serial path: %s\n' "$remote_port" >&2
    exit 1
fi

if ! ssh "${ssh_options[@]}" "$remote_host" \
    "test -f '$remote_dir/.remote-pio-target'"; then
    printf 'The guarded homelab project directory is unavailable: %s:%s\n' \
        "$remote_host" "$remote_dir" >&2
    exit 1
fi

printf 'Synchronizing project to %s:%s\n' "$remote_host" "$remote_dir"
rsync \
    --archive \
    --compress \
    --delete-delay \
    --filter='protect /.remote-pio-target' \
    --exclude='/.git/' \
    --exclude='/.pio/' \
    --exclude='/.cache/' \
    --exclude='/.vscode/' \
    --exclude='__pycache__/' \
    --exclude='*.pyc' \
    --exclude='/compile_commands.json' \
    --exclude='/include/wifi_secrets.h' \
    -e "ssh ${ssh_options[*]}" \
    "$project_dir/" \
    "$remote_host:$remote_dir/"

printf 'Building on the homelab and flashing %s\n' "$remote_port"
ssh "${ssh_options[@]}" "$remote_host" \
    "exec '$remote_dir/homelab/remote_build_upload.sh' '$remote_dir' '$remote_pio' '$remote_env' '$remote_port'"

printf 'Remote ESP32 upload completed successfully.\n'
