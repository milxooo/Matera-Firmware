#include "matera_red.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>

#include <atomic>

#include "config.h"

#if !((defined(CONFIG_BLUEDROID_ENABLED) || defined(CONFIG_NIMBLE_ENABLED)) && __has_include("esp_bt.h"))
#error "Este firmware necesita BLE en el core (BLE provisioning)."
#endif

namespace matera {
namespace red {

namespace {

std::atomic<bool> hayIp{false};
// Un bit por evento (en vez de una sola casilla) para no perder eventos seguidos.
std::atomic<unsigned> eventosProv{0};

// Deben seguir vivos mientras dura el provisioning (el manager guarda los punteros).
std::string popActual;
std::string servicioActual;

void publicar(EventoProv e) { eventosProv.fetch_or(1u << static_cast<unsigned>(e)); }

// OJO: se ejecuta en otra tarea FreeRTOS. Solo marca banderas.
void manejarEvento(arduino_event_t* ev) {
  switch (ev->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: hayIp = true; break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    case ARDUINO_EVENT_WIFI_STA_LOST_IP: hayIp = false; break;
    case ARDUINO_EVENT_PROV_CRED_RECV: publicar(EventoProv::CredencialesRecibidas); break;  // sin imprimir SSID ni clave
    case ARDUINO_EVENT_PROV_CRED_FAIL: publicar(EventoProv::CredencialesFallaron); break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS: publicar(EventoProv::Exito); break;
    case ARDUINO_EVENT_PROV_END: publicar(EventoProv::Fin); break;
    default: break;
  }
}

}  // namespace

void iniciarEventos() { WiFi.onEvent(manejarEvento); }

bool conectado() { return hayIp.load(); }

void conectar() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();  // sin SSID/clave: usa lo guardado en NVS
}

void reintentarConexion() {
  WiFi.disconnect(false, false);  // no apaga el WiFi ni borra credenciales
  WiFi.begin();
}

void iniciarProvisioning(const std::string& codigo, const std::string& mac12) {
  popActual = codigo;
  servicioActual = "PROV_MATERA_" + mac12.substr(mac12.size() - 4);
  WiFi.mode(WIFI_STA);
  // reset_provisioned=false: no borra el WiFi en cada arranque (el ejemplo del core usa true).
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BLE, NETWORK_PROV_SECURITY_1,
                          popActual.c_str(), servicioActual.c_str(), nullptr, nullptr, false);
  Serial.printf("Provisioning BLE iniciado. Servicio: %s\n", servicioActual.c_str());
#if DEBUG_LOG_SECRETS
  Serial.printf("[debug] PoP=%s\n", popActual.c_str());
#endif
  // No se llama a WiFiProv.printQR(): imprimiría el PoP (el código).
}

EventoProv tomarEventoProv() {
  // Orden de entrega: recibidas, fallaron, éxito, fin.
  for (unsigned i = 1; i <= static_cast<unsigned>(EventoProv::Fin); i++) {
    const unsigned bit = 1u << i;
    if (eventosProv.load() & bit) {
      eventosProv.fetch_and(~bit);
      return static_cast<EventoProv>(i);
    }
  }
  return EventoProv::Ninguno;
}

}  // namespace red
}  // namespace matera
