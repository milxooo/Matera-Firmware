// Pruebas de la lógica pura en el PC, sin framework:
//   g++ -std=c++17 -Wall -Wextra -I matera_firmware tests/test_logic.cpp matera_firmware/matera_logic.cpp -o tests/build/test_logic && tests/build/test_logic
#include <cassert>
#include <cstdio>

#include "matera_logic.h"

using namespace matera;

static void test_codigo() {
  std::string out;
  assert(normalizarCodigo("f6dd13792403", out) && out == "f6dd13792403");
  assert(normalizarCodigo("  F6DD13792403\r\n", out) && out == "f6dd13792403");  // a minúsculas, recorta
  assert(!normalizarCodigo("", out));
  assert(!normalizarCodigo("f6dd1379240", out));     // 11
  assert(!normalizarCodigo("f6dd137924033", out));   // 13
  assert(!normalizarCodigo("f6dd1379240g", out));    // no hex
  assert(!normalizarCodigo("f6dd 3792403", out));    // espacio interno
  out = "intacto";
  assert(!normalizarCodigo("zzzzzzzzzzzz", out) && out == "intacto");  // no toca la salida si falla
}

static void test_url() {
  std::string out;
  assert(validarUrlBackend("http://192.168.1.100:5001", out) && out == "http://192.168.1.100:5001");
  assert(validarUrlBackend("  HTTP://host:5001///  ", out) && out == "http://host:5001");
  assert(validarUrlBackend("http://a", out) && out == "http://a");
  assert(!validarUrlBackend("", out));
  assert(!validarUrlBackend("http://", out));
  assert(!validarUrlBackend("http:///", out));
  assert(!validarUrlBackend("http:///ruta", out));
  assert(!validarUrlBackend("http://:5001", out));
  assert(!validarUrlBackend("https://host:5001", out));
  assert(!validarUrlBackend("ftp://host", out));
  assert(!validarUrlBackend("192.168.1.100:5001", out));
  assert(!validarUrlBackend("http://ho st", out));
  assert(!validarUrlBackend("http://host\x01", out));
  assert(!validarUrlBackend("http://" + std::string(200, 'a'), out));  // demasiado larga
}

static void test_payload_qr() {
  const std::string p = payloadQr("246F28AB1234", "f6dd13792403");
  assert(p == "MAT:246F28AB1234:f6dd13792403");
  assert(p.size() == 29);
}

static void test_mac() {
  const uint8_t mac[6] = {0x24, 0x6f, 0x28, 0xab, 0x12, 0x34};
  assert(macAHex12(mac) == "246F28AB1234");
  const uint8_t ceros[6] = {0, 0, 0, 0, 0, 0xFF};
  assert(macAHex12(ceros) == "0000000000FF");
}

static void test_decision_http() {
  assert(decidirActivacion(200) == AccionActivacion::GuardarToken);
  assert(decidirActivacion(404) == AccionActivacion::ErrorCodigoInvalido);
  assert(decidirActivacion(400) == AccionActivacion::ErrorFormato);
  assert(decidirActivacion(409) == AccionActivacion::Reintentar);
  assert(decidirActivacion(500) == AccionActivacion::Reintentar);
  assert(decidirActivacion(503) == AccionActivacion::Reintentar);
  assert(decidirActivacion(-1) == AccionActivacion::Reintentar);   // conexión rechazada
  assert(decidirActivacion(0) == AccionActivacion::Reintentar);
  assert(decidirActivacion(401) == AccionActivacion::ErrorInesperado);
  assert(decidirActivacion(302) == AccionActivacion::ErrorInesperado);
}

static void test_espera() {
  assert(esperaReintentoMs(0) == 5000);
  assert(esperaReintentoMs(1) == 10000);
  assert(esperaReintentoMs(4) == 25000);
  assert(esperaReintentoMs(5) == 30000);
  assert(esperaReintentoMs(1000) == 30000);
  assert(esperaReintentoMs(4294967295u) == 30000);
}

static void test_estados() {
  assert(decidirEstadoInicial(false, false) == Estado::SinCodigo);
  assert(decidirEstadoInicial(false, true) == Estado::SinCodigo);
  assert(decidirEstadoInicial(true, false) == Estado::Provisioning);
  assert(decidirEstadoInicial(true, true) == Estado::Conectando);
  assert(estadoTrasConectar(false) == Estado::Activando);
  assert(estadoTrasConectar(true) == Estado::Operativo);
  assert(std::string(nombreEstado(Estado::Provisioning)) == "PROVISIONING");
}

static void test_token() {
  std::string t;
  assert(extraerToken("{\"token\":\"aaa.bbb.ccc\"}", t) && t == "aaa.bbb.ccc");
  assert(extraerToken("{ \"token\" : \"a-b_c.d\" }\n", t) && t == "a-b_c.d");
  assert(!extraerToken("{\"error\":\"x\"}", t));
  assert(!extraerToken("{\"token\":\"\"}", t));
  assert(!extraerToken("{\"token\":123}", t));
  assert(!extraerToken("{\"token\":\"sin cierre", t));
  assert(!extraerToken("{\"token\":\"a b\"}", t));      // carácter no válido
  assert(!extraerToken("{\"token\":\"a\\\"b\"}", t));   // escapes no soportados
  assert(extraerToken("{\"token\":\"" + std::string(512, 'a') + "\"}", t) && t.size() == 512);
  assert(!extraerToken("{\"token\":\"" + std::string(513, 'a') + "\"}", t));  // > 512 se rechaza
}

static void test_comando() {
  std::string c, a;
  parsearComando("setcode  F6DD13792403 \r\n", c, a);
  assert(c == "setcode" && a == "F6DD13792403");  // el argumento conserva mayúsculas
  parsearComando("STATUS", c, a);
  assert(c == "status" && a.empty());
  parsearComando("   ", c, a);
  assert(c.empty() && a.empty());
  parsearComando("setbackend http://x:1/a b", c, a);
  assert(c == "setbackend" && a == "http://x:1/a b");
}

int main() {
  test_codigo();
  test_url();
  test_payload_qr();
  test_mac();
  test_decision_http();
  test_espera();
  test_estados();
  test_token();
  test_comando();
  std::printf("OK: todas las pruebas de matera_logic pasaron\n");
  return 0;
}
