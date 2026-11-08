// Firmware Matera (ESP32-S3): vinculación con el backend.
// Estados: SIN_CODIGO -> PROVISIONING -> CONECTANDO -> ACTIVANDO -> OPERATIVO (+ ERROR).
#include "config.h"
#include "matera_activation.h"
#include "matera_console.h"
#include "matera_device.h"
#include "matera_display.h"
#include "matera_display_oled.h"
#include "matera_logic.h"
#include "matera_red.h"
#include "matera_storage.h"

using namespace matera;

// Si la OLED no responde, PantallaOled cae a Serial y el arranque no se cuelga.
static PantallaOled pantallaOled;
static Pantalla* pantalla = &pantallaOled;

static Estado estadoActual = Estado::SinCodigo;
static std::string mac12;
static unsigned long inicioIntentoWifi = 0;
static unsigned long reinicioPorFalloAt = 0;  // 0 = no programado
static EstadoBoton boton;
static unsigned intentosActivacion = 0;
static unsigned long proximoIntentoAt = 0;

static void cambiarEstado(Estado nuevo) {
  estadoActual = nuevo;
  Serial.printf("Estado: %s\n", nombreEstado(nuevo));
  switch (nuevo) {
    case Estado::SinCodigo: pantalla->mostrarEstado("Falta codigo\n(setcode por Serial)"); break;
    case Estado::Provisioning: break;  // la pantalla muestra el QR (se dibuja en entrarEnProvisioning)
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
  // El QR solo se muestra en este estado: MAT:<MAC12>:<CODIGO> (codigo en minusculas, tal cual).
  pantalla->mostrarQr(payloadQr(mac12, almacen::codigo()).c_str());
  Serial.println("Da el WiFi con la app 'ESP BLE Provisioning' (PoP = codigo de emparejamiento).");
}

static void entrarEnActivando() {
  intentosActivacion = 0;
  proximoIntentoAt = millis();  // primer intento de inmediato
  cambiarEstado(Estado::Activando);
}

// Tras conectar: ACTIVANDO si falta el token, OPERATIVO si ya existe.
static void alConectar() {
  Serial.println("WiFi conectado.");
  if (estadoTrasConectar(almacen::tieneToken()) == Estado::Activando) entrarEnActivando();
  else cambiarEstado(Estado::Operativo);
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
  if (red::conectado()) alConectar();
}

static void atenderConectando() {
  if (red::conectado()) {
    alConectar();
    return;
  }
  if (millis() - inicioIntentoWifi >= WIFI_CONNECT_TIMEOUT_MS) {
    // No se borran las credenciales aunque falle: puede ser una caida temporal del router.
    Serial.println("Sin conexion WiFi todavia; reintentando (usa 'reset' para volver a emparejar).");
    inicioIntentoWifi = millis();
    red::reintentarConexion();
  }
}

static void atenderActivando() {
  if (!red::conectado()) {  // se cayo el WiFi: volver a conectar sin tocar credenciales
    entrarEnConectando();
    return;
  }
  if (static_cast<long>(millis() - proximoIntentoAt) < 0) return;

  std::string cuerpo;
  const int http = activacion::intentar(almacen::urlBackendActiva(), mac12, almacen::codigo(), cuerpo);
  switch (decidirActivacion(http)) {
    case AccionActivacion::GuardarToken: {
      std::string token;
      if (extraerToken(cuerpo, token) && almacen::guardarToken(token)) {
        Serial.println("Activacion correcta: token de dispositivo guardado.");
#if DEBUG_LOG_SECRETS
        Serial.printf("[debug] token=%s\n", token.c_str());
#endif
        cambiarEstado(Estado::Operativo);
        return;
      }
      // 200 con cuerpo inesperado o token demasiado largo: no tiene sentido reintentar igual.
      Serial.println("ERROR: respuesta 200 sin un token valido (o supera 512 bytes).");
      cambiarEstado(Estado::Error);
      return;
    }
    case AccionActivacion::ErrorCodigoInvalido:
      Serial.println("ERROR 404: el codigo no corresponde a esta MAC. Usa 'setcode <codigo>' o 'factory-reset'.");
      cambiarEstado(Estado::Error);
      return;
    case AccionActivacion::ErrorFormato:
      Serial.println("ERROR 400: el backend rechazo el formato de MAC/codigo (error de programacion).");
      cambiarEstado(Estado::Error);
      return;
    case AccionActivacion::ErrorInesperado:
      Serial.printf("ERROR: respuesta HTTP inesperada (%d). Revisa la URL del backend (status/setbackend).\n", http);
      cambiarEstado(Estado::Error);
      return;
    case AccionActivacion::Reintentar: {
      const unsigned long espera = esperaReintentoMs(intentosActivacion);
      if (http == 409) Serial.printf("Matera aun no vinculada desde la app (409). Reintento en %lu s.\n", espera / 1000);
      else if (http <= 0) Serial.printf("Sin respuesta del backend (error %d). Reintento en %lu s.\n", http, espera / 1000);
      else Serial.printf("Backend respondio %d. Reintento en %lu s.\n", http, espera / 1000);
      intentosActivacion++;
      proximoIntentoAt = millis() + espera;
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  almacen::iniciar();
  pantalla->iniciar();
  red::iniciarEventos();
  pinMode(PIN_BOTON_RESET, INPUT_PULLUP);
  iniciarBoton(boton, digitalRead(PIN_BOTON_RESET) == LOW);
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
    case Estado::Activando: atenderActivando(); break;
    default: break;  // OPERATIVO: aqui iria el envio de lecturas (ver README); ERROR: espera reset/setcode
  }
  // Reset de emparejamiento: BOOT (GPIO0) mantenido BOTON_RESET_MS. Se revisa en todos los estados.
  if (evaluarBoton(boton, digitalRead(PIN_BOTON_RESET) == LOW, millis(), BOTON_RESET_MS)) {
    Serial.println("Boton BOOT mantenido: reset de emparejamiento.");
    dispositivo::reiniciarEmparejamiento(false);
  }
  EstadoConsola ctx = {nombreEstado(estadoActual), red::conectado()};
  consola::atender(ctx);
}
