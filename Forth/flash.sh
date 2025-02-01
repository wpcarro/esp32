#!/usr/bin/env bash

set -euo pipefail

file="$HOME/programming/esp32/Forth/Forth.ino"

echo "Flashing..."

# Format the codes
clang-format -i --style=file $file

# Build
arduino-cli compile -b esp32:esp32:esp32 $file
arduino-cli upload -b esp32:esp32:esp32 $file -p /dev/cu.usbserial-0001

echo "Firmware updated!"
