#!/usr/bin/env bash

set -euo pipefail

file="$HOME/programming/esp32/Forth/main.fth"
device="/dev/cu.usbserial-0001"
echo "Uploading \"$(basename $file)\" -> $device"
cat $file > $device
echo "Done."
