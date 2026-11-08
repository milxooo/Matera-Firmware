#include "matera_storage.h"

#include <Preferences.h>

#include "config.h"
#include "matera_logic.h"

namespace matera {
namespace almacen {

namespace {
const char* ESPACIO = "matera";
const char* K_CODIGO = "codigo";
const char* K_TOKEN = "token";
const char* K_URL = "backend";

Preferences prefs;

std::string leer(const char* clave) {
  prefs.begin(ESPACIO, true);
  const String valor = prefs.getString(clave, "");
  prefs.end();
  return std::string(valor.c_str());
}

bool escribir(const char* clave, const std::string& valor) {
  prefs.begin(ESPACIO, false);
  const size_t n = prefs.putString(clave, valor.c_str());
  prefs.end();
  return n == valor.size();
}

void borrar(const char* clave) {
  prefs.begin(ESPACIO, false);
  prefs.remove(clave);
  prefs.end();
}
}  // namespace

void iniciar() {
  // Crea el espacio de nombres si no existe, para que las lecturas de solo lectura no fallen.
  prefs.begin(ESPACIO, false);
  prefs.end();
}

bool tieneCodigo() { return !leer(K_CODIGO).empty(); }
std::string codigo() { return leer(K_CODIGO); }
bool guardarCodigo(const std::string& c) { return !c.empty() && escribir(K_CODIGO, c); }

bool tieneToken() { return !leer(K_TOKEN).empty(); }
std::string token() { return leer(K_TOKEN); }
bool guardarToken(const std::string& t) {
  if (t.empty() || t.size() > TOKEN_MAX_LEN) return false;
  return escribir(K_TOKEN, t);
}

std::string urlBackendActiva() {
  const std::string guardada = leer(K_URL);
  return guardada.empty() ? std::string(BACKEND_BASE_URL) : guardada;
}
bool guardarUrlBackend(const std::string& url) { return !url.empty() && escribir(K_URL, url); }

void borrarToken() { borrar(K_TOKEN); }
void borrarCodigo() { borrar(K_CODIGO); }

}  // namespace almacen
}  // namespace matera
