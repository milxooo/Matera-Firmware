#include "matera_display.h"

#include <Arduino.h>

namespace matera {

void PantallaSerial::iniciar() { Serial.println("[pantalla] modo solo-Serial"); }

void PantallaSerial::mostrarEstado(const char* texto) { Serial.printf("[pantalla] %s\n", texto); }

void PantallaSerial::mostrarQr(const char* payload) {
  (void)payload;  // contiene el código: no se imprime
  Serial.println("[pantalla] (QR de emparejamiento; no se muestra en modo solo-Serial)");
}

}  // namespace matera
