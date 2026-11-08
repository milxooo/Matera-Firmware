#include "matera_device.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_mac.h>
#include <esp_wifi.h>

#include "matera_logic.h"
#include "matera_storage.h"

namespace matera {
namespace dispositivo {

std::string mac12() {
  uint8_t mac[6];
  // MAC base de fábrica (eFuse): estable aunque el WiFi no esté iniciado.
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  return macAHex12(mac);
}

bool wifiGuardado() {
  WiFi.mode(WIFI_STA);  // inicializa el driver para poder leer la configuración guardada
  wifi_config_t conf;
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return false;
  return conf.sta.ssid[0] != 0;
}

void reiniciar() {
  Serial.println("Reiniciando...");
  Serial.flush();
  delay(300);
  ESP.restart();
  while (true) {}  // ESP.restart no vuelve; esto satisface [[noreturn]]
}

void reiniciarEmparejamiento(bool factory) {
  WiFi.mode(WIFI_STA);        // esp_wifi_restore necesita el driver iniciado
  esp_wifi_restore();         // borra el WiFi guardado en NVS (verificado en STA::erase del core 3.3.12)
  almacen::borrarToken();
  if (factory) almacen::borrarCodigo();
  Serial.println(factory ? "Factory reset: WiFi, token y codigo borrados." : "Reset: WiFi y token borrados (el codigo se conserva).");
  reiniciar();
}

}  // namespace dispositivo
}  // namespace matera
