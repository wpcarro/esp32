#!/usr/bin/env bash

set -euo pipefail

file="$HOME/programming/esp32/Forth/Forth.ino"

echo "[flash.sh] Flashing..."

# Format the codes
echo "[flash.sh] Formatting source code..."
clang-format -i --style=file $file

# Build
echo "[flash.sh] Compiling..."
arduino-cli compile -b esp32:esp32:esp32 $file

# NOTE: We need the empty password field. Otherwise, arduino-cli will prompt us
# for the password.
echo "[flash.sh] Uploading (OTA)..."
arduino-cli upload -b esp32:esp32:esp32 $file -p 172.20.10.5 --upload-field password=""

echo "[flash.sh] Firmware updated!"
