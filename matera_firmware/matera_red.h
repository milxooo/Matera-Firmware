// WiFi y BLE provisioning. Los eventos del core llegan desde otra tarea FreeRTOS: aquí solo
// se marcan banderas atómicas y el loop principal las consume con tomarEventoProv().
#pragma once

#include <string>

namespace matera {
namespace red {

enum class EventoProv { Ninguno, CredencialesRecibidas, CredencialesFallaron, Exito, Fin };

// Registra el manejador de eventos de WiFi (llamar una sola vez en setup).
void iniciarEventos();

// ¿Hay conexión WiFi con IP?
bool conectado();

// Conecta con el WiFi guardado (WiFi.begin() sin argumentos). No borra credenciales.
void conectar();
// Reintenta la conexión sin borrar credenciales.
void reintentarConexion();

// Inicia el BLE provisioning (seguridad 1, PoP = código). El servicio se llama
// PROV_MATERA_ + los últimos 4 hex de la MAC. Al terminar, el core libera el BLE.
void iniciarProvisioning(const std::string& codigo, const std::string& mac12);

// Devuelve (y consume) el siguiente evento de provisioning pendiente.
EventoProv tomarEventoProv();

}  // namespace red
}  // namespace matera
