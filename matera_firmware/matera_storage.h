// Persistencia en NVS (Preferences, espacio de nombres "matera").
// Guarda código, token y URL del backend. Las credenciales de WiFi las guarda el WiFi del core.
// NVS no está cifrado por defecto: limitación aceptada para el proyecto académico.
#pragma once

#include <string>

namespace matera {
namespace almacen {

void iniciar();

bool tieneCodigo();
std::string codigo();                      // "" si no hay
bool guardarCodigo(const std::string& c);  // ya normalizado (12 hex minúsculas)

bool tieneToken();
std::string token();                       // "" si no hay
bool guardarToken(const std::string& t);   // rechaza vacíos y más largos que TOKEN_MAX_LEN

// URL activa: la guardada con `setbackend` o, si no hay, BACKEND_BASE_URL de config.h.
std::string urlBackendActiva();
bool guardarUrlBackend(const std::string& url);  // ya validada

void borrarToken();
void borrarCodigo();

}  // namespace almacen
}  // namespace matera
