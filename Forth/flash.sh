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

if [ -e /dev/cu.usbserial-0001 ]; then
  echo "[flash.sh] Detected wired connection."

  echo "[flash.sh] Killing existing serial connections..."
  pkill picocom || echo "[flash.sh] Could not find any"

  echo "[flash.sh] Uploading..."
  arduino-cli upload -b esp32:esp32:esp32 $file -p /dev/cu.usbserial-0001
else
  echo "[flash.sh] No wired connection found. Attempting OTA update instead..."

  # NOTE: We need the empty password field. Otherwise, arduino-cli will prompt us
  # for the password.
  echo "[flash.sh] Uploading (OTA)..."
  arduino-cli upload -b esp32:esp32:esp32 $file -p 172.20.10.5 --upload-field password=""
fi

echo "[flash.sh] Firmware updated!"
