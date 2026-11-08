// Lógica pura del firmware: sin Arduino.h ni hardware, para poder probarla en el PC
// con g++ (ver tests/test_logic.cpp).
#pragma once

#include <cstdint>
#include <string>

namespace matera {

// Largo máximo del token de dispositivo que se acepta y guarda en NVS.
constexpr size_t TOKEN_MAX_LEN = 512;
// Largo máximo de la URL del backend.
constexpr size_t URL_MAX_LEN = 100;

enum class Estado { SinCodigo, Provisioning, Conectando, Activando, Operativo, Error };

const char* nombreEstado(Estado estado);

// Estado al arrancar según lo guardado. Si hay WiFi guardado se pasa por CONECTANDO
// (después: estadoTrasConectar). Sin código no se puede emparejar ni activar.
Estado decidirEstadoInicial(bool tieneCodigo, bool tieneWifi);
// Una vez conectado a la red: ACTIVANDO si falta el token, OPERATIVO si ya existe.
Estado estadoTrasConectar(bool tieneToken);

// MAC de 6 bytes -> 12 hex en MAYÚSCULAS sin separadores (formato del backend).
std::string macAHex12(const uint8_t mac[6]);

// Valida el código de emparejamiento (12 hex, se ignoran espacios en los extremos) y lo
// devuelve en minúsculas, que es como lo guarda y lo envía el firmware.
bool normalizarCodigo(const std::string& entrada, std::string& salida);

// Valida la URL del backend: debe empezar por http:// y tener host. Quita "/" finales.
bool validarUrlBackend(const std::string& entrada, std::string& salida);

// Contenido del QR: MAT:<MAC12>:<CODIGO> (el código tal cual, en minúsculas).
std::string payloadQr(const std::string& mac12, const std::string& codigo);

// Capacidad en modo byte de un QR versión 2 con corrección de errores L. La biblioteca QRCode
// NO valida la capacidad (con más bytes "tiene éxito" sin avisar), así que se comprueba antes.
constexpr size_t QR_MAX_BYTES = 32;
bool payloadQrCabe(const std::string& payload);

// Qué hacer según el resultado de POST /dispositivos/activar.
// codigoHttp <= 0 es un error de red (HTTPClient devuelve negativos).
enum class AccionActivacion { GuardarToken, Reintentar, ErrorCodigoInvalido, ErrorFormato, ErrorInesperado };
AccionActivacion decidirActivacion(int codigoHttp);

// Espera antes del siguiente reintento: 5 s, subiendo de 5 en 5 hasta 30 s.
// intento = 0 para el primer reintento.
unsigned long esperaReintentoMs(unsigned intento);

// Extrae el JWT de {"token":"..."}. Falla si falta, está vacío, supera maxLen o trae
// caracteres que no son de un JWT (A-Z a-z 0-9 . _ -).
bool extraerToken(const std::string& cuerpo, std::string& token, size_t maxLen = TOKEN_MAX_LEN);

// Separa una línea de consola en comando (en minúsculas) y argumento (sin espacios extremos).
void parsearComando(const std::string& linea, std::string& comando, std::string& argumento);

// Detección de "mantener pulsado": se llama en cada vuelta del loop con el estado del botón.
// Devuelve true UNA vez cuando se cumplió `umbralMs` de pulsación continua. Si el botón ya
// estaba pulsado al arrancar (arranque con BOOT en bajo), hay que soltarlo antes de contar.
struct EstadoBoton {
  bool contando = false;
  bool disparado = false;
  bool esperarSoltar = false;
  unsigned long inicio = 0;
};
void iniciarBoton(EstadoBoton& b, bool pulsadoAlArrancar);
bool evaluarBoton(EstadoBoton& b, bool pulsado, unsigned long ahoraMs, unsigned long umbralMs);

}  // namespace matera
