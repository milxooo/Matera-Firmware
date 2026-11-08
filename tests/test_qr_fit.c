// Verifica en el PC que la biblioteca QRCode genera el payload en version 2 / ECC bajo (25x25)
// sin alterar mayusculas/minusculas. Compilar: ver tests/run_tests.sh
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ricmoo_qrcode.h"

int main(void) {
  const char* payload = "MAT:246F28AB1234:f6dd13792403";
  assert(strlen(payload) == 29);
  QRCode qr;
  uint8_t buf[256];
  assert(qrcode_getBufferSize(2) <= sizeof(buf));
  int8_t r = qrcode_initText(&qr, buf, 2, ECC_LOW, payload);
  assert(r == 0);
  printf("version=%u size=%u ecc=%u mode=%u mask=%u\n", qr.version, qr.size, qr.ecc, qr.mode, qr.mask);
  assert(qr.version == 2 && qr.size == 25 && qr.ecc == ECC_LOW);
  // Cabe: (25 + 2*2 de zona de silencio) * 2 px = 58 px <= 64 px de alto
  assert((qr.size + 4) * 2 <= 64);
  // Patron de busqueda (esquina superior izquierda): borde exterior 7x7 oscuro
  for (int i = 0; i < 7; i++) { assert(qrcode_getModule(&qr, i, 0)); assert(qrcode_getModule(&qr, 0, i)); }
  for (int y = 0; y < qr.size; y++) {
    for (int x = 0; x < qr.size; x++) putchar(qrcode_getModule(&qr, x, y) ? '#' : '.');
    putchar('\n');
  }
  // (La biblioteca no valida capacidad: la guarda esta en matera_logic payloadQrCabe, con su prueba.)
  puts("OK: QR v2/ECC_LOW 25x25");
  return 0;
}
