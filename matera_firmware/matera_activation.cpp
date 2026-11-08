#include "matera_activation.h"

#include <HTTPClient.h>
#include <WiFiClient.h>

#include "config.h"

namespace matera {
namespace activacion {

int intentar(const std::string& urlBase, const std::string& mac12, const std::string& codigo, std::string& cuerpo) {
  cuerpo.clear();
  WiFiClient cliente;
  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  const std::string url = urlBase + RUTA_ACTIVAR;
  if (!http.begin(cliente, url.c_str())) return -1;
  http.addHeader("Content-Type", "application/json");
  // MAC en MAYUSCULAS y codigo en minusculas, tal cual estan guardados (solo hex: no necesita escapes).
  const std::string json = "{\"mac\":\"" + mac12 + "\",\"codigo\":\"" + codigo + "\"}";
  const int http_code = http.POST(json.c_str());
  if (http_code > 0) cuerpo = http.getString().c_str();
  http.end();
  return http_code;
}

}  // namespace activacion
}  // namespace matera
