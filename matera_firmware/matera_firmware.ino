// Firmware Matera (ESP32-S3): vinculación con el backend.
// Etapa 1: almacenamiento y consola Serial. Las etapas siguientes agregan WiFi/BLE,
// activación, reset por botón y pantalla OLED.
#include "config.h"
#include "matera_console.h"
#include "matera_device.h"
#include "matera_display.h"
#include "matera_logic.h"
#include "matera_storage.h"

using namespace matera;

static PantallaSerial pantallaSerial;
static Pantalla* pantalla = &pantallaSerial;
static Estado estadoActual = Estado::SinCodigo;

void setup() {
  Serial.begin(115200);
  delay(500);
  almacen::iniciar();
  pantalla->iniciar();

  Serial.println();
  Serial.println("=== Matera firmware ===");
  Serial.printf("MAC12: %s\n", dispositivo::mac12().c_str());

  estadoActual = decidirEstadoInicial(almacen::tieneCodigo(), dispositivo::wifiGuardado());
  Serial.printf("Estado inicial: %s\n", nombreEstado(estadoActual));
  if (estadoActual == Estado::SinCodigo) {
    Serial.println("Falta el codigo de emparejamiento. En el PC:");
    Serial.printf("  python -m backend.materas.cli %s\n", dispositivo::mac12().c_str());
    Serial.println("y luego aqui: setcode <codigo>");
  }
  Serial.println("Escribe help para ver los comandos.");
}

void loop() {
  EstadoConsola ctx = {nombreEstado(estadoActual), false};
  consola::atender(ctx);
}
