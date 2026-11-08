#!/usr/bin/env bash
# Pruebas en el PC (sin placa ni framework). Uso: tests/run_tests.sh
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p tests/build
g++ -std=c++17 -Wall -Wextra -I matera_firmware tests/test_logic.cpp matera_firmware/matera_logic.cpp -o tests/build/test_logic
tests/build/test_logic

# Prueba del QR con la copia local de QRCode (el mismo codigo que se graba)
QR=matera_firmware
if [ -f "$QR/ricmoo_qrcode.c" ]; then
  gcc -std=gnu11 -w -I "$QR" tests/test_qr_fit.c "$QR/ricmoo_qrcode.c" -o tests/build/test_qr_fit
  tests/build/test_qr_fit | tail -1
else
  echo "falta ricmoo_qrcode.c"
fi
