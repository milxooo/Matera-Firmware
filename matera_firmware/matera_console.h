// Consola Serial no bloqueante: acumula caracteres hasta fin de línea y ejecuta el comando.
// Comandos: help, status, setcode <12 hex>, setbackend <url>, i2cscan, reset, factory-reset.
#pragma once

namespace matera {

struct EstadoConsola {
  const char* estado;     // nombre del estado actual de la máquina de estados
  bool wifiConectado;     // ¿hay conexión WiFi ahora mismo?
};

namespace consola {
// Llamar en cada vuelta del loop; no bloquea.
void atender(const EstadoConsola& ctx);
}  // namespace consola

}  // namespace matera
