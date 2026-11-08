#include "matera_logic.h"

namespace matera {

namespace {

bool esEspacio(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string recortar(const std::string& s) {
  size_t ini = 0;
  size_t fin = s.size();
  while (ini < fin && esEspacio(s[ini])) ini++;
  while (fin > ini && esEspacio(s[fin - 1])) fin--;
  return s.substr(ini, fin - ini);
}

char aMinuscula(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

bool esHex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }

bool esCaracterJwt(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' ||
         c == '-';
}

}  // namespace

const char* nombreEstado(Estado estado) {
  switch (estado) {
    case Estado::SinCodigo: return "SIN_CODIGO";
    case Estado::Provisioning: return "PROVISIONING";
    case Estado::Conectando: return "CONECTANDO";
    case Estado::Activando: return "ACTIVANDO";
    case Estado::Operativo: return "OPERATIVO";
    case Estado::Error: return "ERROR";
  }
  return "?";
}

Estado decidirEstadoInicial(bool tieneCodigo, bool tieneWifi) {
  if (!tieneCodigo) return Estado::SinCodigo;
  if (!tieneWifi) return Estado::Provisioning;
  return Estado::Conectando;
}

Estado estadoTrasConectar(bool tieneToken) { return tieneToken ? Estado::Operativo : Estado::Activando; }

std::string macAHex12(const uint8_t mac[6]) {
  static const char* DIGITOS = "0123456789ABCDEF";  // (no llamarlo HEX: Arduino.h lo define como macro)
  std::string salida;
  for (int i = 0; i < 6; i++) {
    salida += DIGITOS[mac[i] >> 4];
    salida += DIGITOS[mac[i] & 0x0F];
  }
  return salida;
}

bool normalizarCodigo(const std::string& entrada, std::string& salida) {
  const std::string limpio = recortar(entrada);
  if (limpio.size() != 12) return false;
  std::string resultado;
  for (char c : limpio) {
    if (!esHex(c)) return false;
    resultado += aMinuscula(c);
  }
  salida = resultado;
  return true;
}

bool validarUrlBackend(const std::string& entrada, std::string& salida) {
  std::string url = recortar(entrada);
  static const std::string ESQUEMA = "http://";
  if (url.size() <= ESQUEMA.size() || url.size() > URL_MAX_LEN) return false;
  for (size_t i = 0; i < ESQUEMA.size(); i++) {
    if (aMinuscula(url[i]) != ESQUEMA[i]) return false;
  }
  for (char c : url) {
    if (c <= ' ' || c == 0x7F) return false;  // sin espacios ni caracteres de control
  }
  while (url.size() > ESQUEMA.size() && url.back() == '/') url.pop_back();
  if (url.size() == ESQUEMA.size()) return false;  // solo "http://" y barras
  const char primero = url[ESQUEMA.size()];
  if (primero == '/' || primero == ':' || primero == '?' || primero == '#') return false;  // falta el host
  salida = "http://" + url.substr(ESQUEMA.size());
  return true;
}

std::string payloadQr(const std::string& mac12, const std::string& codigo) { return "MAT:" + mac12 + ":" + codigo; }

AccionActivacion decidirActivacion(int codigoHttp) {
  if (codigoHttp <= 0) return AccionActivacion::Reintentar;
  if (codigoHttp == 200) return AccionActivacion::GuardarToken;
  if (codigoHttp == 404) return AccionActivacion::ErrorCodigoInvalido;
  if (codigoHttp == 400) return AccionActivacion::ErrorFormato;
  if (codigoHttp == 409 || codigoHttp >= 500) return AccionActivacion::Reintentar;
  return AccionActivacion::ErrorInesperado;
}

unsigned long esperaReintentoMs(unsigned intento) {
  const unsigned long espera = 5000UL + 5000UL * (intento > 5 ? 5 : intento);
  return espera > 30000UL ? 30000UL : espera;
}

bool extraerToken(const std::string& cuerpo, std::string& token, size_t maxLen) {
  size_t pos = cuerpo.find("\"token\"");
  if (pos == std::string::npos) return false;
  pos += 7;
  while (pos < cuerpo.size() && esEspacio(cuerpo[pos])) pos++;
  if (pos >= cuerpo.size() || cuerpo[pos] != ':') return false;
  pos++;
  while (pos < cuerpo.size() && esEspacio(cuerpo[pos])) pos++;
  if (pos >= cuerpo.size() || cuerpo[pos] != '"') return false;
  pos++;
  const size_t fin = cuerpo.find('"', pos);
  if (fin == std::string::npos) return false;
  const std::string valor = cuerpo.substr(pos, fin - pos);
  if (valor.empty() || valor.size() > maxLen) return false;
  for (char c : valor) {
    if (!esCaracterJwt(c)) return false;
  }
  token = valor;
  return true;
}

void parsearComando(const std::string& linea, std::string& comando, std::string& argumento) {
  const std::string limpia = recortar(linea);
  size_t corte = 0;
  while (corte < limpia.size() && !esEspacio(limpia[corte])) corte++;
  comando.clear();
  for (size_t i = 0; i < corte; i++) comando += aMinuscula(limpia[i]);
  argumento = recortar(limpia.substr(corte));
}

}  // namespace matera
