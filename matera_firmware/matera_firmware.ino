// Firmware Matera (ESP32-S3): vinculación con el backend.
// Estados: SIN_CODIGO -> PROVISIONING -> CONECTANDO -> ACTIVANDO -> OPERATIVO (+ ERROR).
#include "config.h"
#include "matera_console.h"
#include "matera_device.h"
#include "matera_display.h"
#include "matera_logic.h"
#include "matera_red.h"
#include "matera_storage.h"

using namespace matera;

static PantallaSerial pantallaSerial;
static Pantalla* pantalla = &pantallaSerial;

static Estado estadoActual = Estado::SinCodigo;
static std::string mac12;
static unsigned long inicioIntentoWifi = 0;
static unsigned long reinicioPorFalloAt = 0;  // 0 = no programado

static void cambiarEstado(Estado nuevo) {
  estadoActual = nuevo;
  Serial.printf("Estado: %s\n", nombreEstado(nuevo));
  switch (nuevo) {
    case Estado::SinCodigo: pantalla->mostrarEstado("Falta codigo\n(setcode por Serial)"); break;
    case Estado::Provisioning: pantalla->mostrarEstado("Emparejando..."); break;
    case Estado::Conectando: pantalla->mostrarEstado("Conectando WiFi"); break;
    case Estado::Activando: pantalla->mostrarEstado("Activando..."); break;
    case Estado::Operativo: pantalla->mostrarEstado("Operativo"); break;
    case Estado::Error: pantalla->mostrarEstado("ERROR (ver Serial)"); break;
  }
}

static void entrarEnConectando() {
  cambiarEstado(Estado::Conectando);
  inicioIntentoWifi = millis();
  red::conectar();
}

static void entrarEnProvisioning() {
  cambiarEstado(Estado::Provisioning);
  red::iniciarProvisioning(almacen::codigo(), mac12);
  Serial.println("Da el WiFi con la app 'ESP BLE Provisioning' (PoP = codigo de emparejamiento).");
}

static void atenderProvisioning() {
  switch (red::tomarEventoProv()) {
    case red::EventoProv::CredencialesRecibidas: Serial.println("Provisioning: credenciales WiFi recibidas."); break;
    case red::EventoProv::CredencialesFallaron:
      // El BLE no se libera sin reiniciar: se borra el WiFi fallido y se reinicia, conservando el codigo.
      Serial.println("Provisioning: fallo (clave incorrecta o red no encontrada). Reiniciando para reintentar...");
      reinicioPorFalloAt = millis() + 3000;
      break;
    case red::EventoProv::Exito: Serial.println("Provisioning: exito."); break;
    case red::EventoProv::Fin: Serial.println("Provisioning: finalizado (BLE liberado)."); break;
    default: break;
  }
  if (reinicioPorFalloAt != 0 && static_cast<long>(millis() - reinicioPorFalloAt) >= 0) {
    dispositivo::reiniciarEmparejamiento(false);
  }
  if (red::conectado()) {
    Serial.println("WiFi conectado.");
    cambiarEstado(estadoTrasConectar(almacen::tieneToken()));
  }
}

static void atenderConectando() {
  if (red::conectado()) {
    Serial.println("WiFi conectado.");
    cambiarEstado(estadoTrasConectar(almacen::tieneToken()));
    return;
  }
  if (millis() - inicioIntentoWifi >= WIFI_CONNECT_TIMEOUT_MS) {
    // No se borran las credenciales aunque falle: puede ser una caida temporal del router.
    Serial.println("Sin conexion WiFi todavia; reintentando (usa 'reset' para volver a emparejar).");
    inicioIntentoWifi = millis();
    red::reintentarConexion();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  almacen::iniciar();
  pantalla->iniciar();
  red::iniciarEventos();
  mac12 = dispositivo::mac12();

  Serial.println();
  Serial.println("=== Matera firmware ===");
  Serial.printf("MAC12: %s\n", mac12.c_str());
  Serial.println("Escribe help para ver los comandos.");

  const Estado inicial = decidirEstadoInicial(almacen::tieneCodigo(), dispositivo::wifiGuardado());
  switch (inicial) {
    case Estado::SinCodigo:
      cambiarEstado(Estado::SinCodigo);
      Serial.println("Falta el codigo de emparejamiento. En el PC:");
      Serial.printf("  python -m backend.materas.cli %s\n", mac12.c_str());
      Serial.println("y luego aqui: setcode <codigo>");
      break;
    case Estado::Provisioning: entrarEnProvisioning(); break;
    default: entrarEnConectando(); break;
  }
}

void loop() {
  switch (estadoActual) {
    case Estado::Provisioning: atenderProvisioning(); break;
    case Estado::Conectando: atenderConectando(); break;
    default: break;  // ACTIVANDO y OPERATIVO se agregan en las etapas siguientes
  }
  EstadoConsola ctx = {nombreEstado(estadoActual), red::conectado()};
  consola::atender(ctx);
}
