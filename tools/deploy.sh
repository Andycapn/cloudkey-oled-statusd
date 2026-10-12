#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
BUILD_DIR="$PROJECT_DIR/cmake-build-production"
SERVICE_NAME=${1:-oled-statusd.service}

if [ "$#" -gt 1 ]; then
    echo "Usage: sudo $0 [systemd-service]" >&2
    exit 2
fi

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this script as root: sudo $0 [systemd-service]" >&2
    exit 1
fi

for command in cmake systemctl; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "Required command not found: $command" >&2
        exit 1
    fi
done

if ! systemctl cat "$SERVICE_NAME" >/dev/null 2>&1; then
    echo "Systemd unit not found: $SERVICE_NAME" >&2
    echo "Pass the installed unit name as the first argument." >&2
    exit 1
fi

echo "Configuring hardware build..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DSIMULATOR=OFF \
    -DCMAKE_BUILD_TYPE=Release

echo "Building oled-statusd..."
cmake --build "$BUILD_DIR" --target oled-statusd

echo "Installing executable and icons..."
install -d /usr/local/bin /usr/local/share/oled-status/icons
install -m 755 "$BUILD_DIR/oled-statusd" /usr/local/bin/oled-statusd
for icon in "$PROJECT_DIR"/assets/icons/*.bin; do
    install -m 644 "$icon" /usr/local/share/oled-status/icons/
done

echo "Restarting $SERVICE_NAME..."
systemctl restart "$SERVICE_NAME"
systemctl is-active --quiet "$SERVICE_NAME"
echo "Deploy complete; $SERVICE_NAME is active."