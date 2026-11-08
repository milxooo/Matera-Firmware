// Operaciones que tocan hardware/SO: MAC, WiFi guardado y reset de emparejamiento.
#pragma once

#include <string>

namespace matera {
namespace dispositivo {

std::string mac12();  // MAC del chip, 12 hex en MAYÚSCULAS

// ¿Hay una red WiFi guardada en NVS? (el WiFi del core las guarda al provisionar)
bool wifiGuardado();

// Borra el WiFi guardado (esp_wifi_restore) y el token. Con `factory` borra también el código.
// Termina con ESP.restart(): la memoria de BLE (FREE_BLE) solo se recupera reiniciando.
// Al arrancar de nuevo, el estado PROVISIONING (o SIN_CODIGO) sale de no tener WiFi/código.
[[noreturn]] void reiniciarEmparejamiento(bool factory);

// Reinicia tras un breve retardo para que Serial termine de enviar.
[[noreturn]] void reiniciar();

}  // namespace dispositivo
}  // namespace matera
