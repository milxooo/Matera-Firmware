#include "matera_console.h"

#include <Arduino.h>
#include <Wire.h>

#include <string>

#include "config.h"
#include "matera_device.h"
#include "matera_logic.h"
#include "matera_storage.h"

namespace matera {
namespace consola {

namespace {

std::string linea;

const char* si(bool v) { return v ? "si" : "no"; }

void ayuda() {
  Serial.println("Comandos:");
  Serial.println("  help                  esta ayuda");
  Serial.println("  status                estado, MAC, URL del backend y que datos hay guardados");
  Serial.println("  setcode <12 hex>      guarda el codigo de emparejamiento y reinicia");
  Serial.println("  setbackend <url>      URL del backend (http://...); tiene prioridad sobre config.h");
  Serial.println("  i2cscan               lista los dispositivos I2C (verifica la pantalla)");
  Serial.println("  reset                 borra WiFi y token (conserva el codigo) y reinicia");
  Serial.println("  factory-reset         borra WiFi, token y codigo, y reinicia");
}

void estado(const EstadoConsola& ctx) {
  // Nunca imprime valores secretos (codigo, token, credenciales WiFi).
  Serial.printf("Estado:   %s\n", ctx.estado);
  Serial.printf("MAC12:    %s\n", dispositivo::mac12().c_str());
  Serial.printf("Backend:  %s\n", almacen::urlBackendActiva().c_str());
  Serial.printf("Codigo:   %s\n", si(almacen::tieneCodigo()));
  Serial.printf("WiFi:     guardado=%s conectado=%s\n", si(dispositivo::wifiGuardado()), si(ctx.wifiConectado));
  Serial.printf("Token:    %s\n", si(almacen::tieneToken()));
}

void i2cscan() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Serial.printf("Escaneando I2C (SDA=GPIO%d, SCL=GPIO%d)...\n", PIN_I2C_SDA, PIN_I2C_SCL);
  int encontrados = 0;
  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  encontrado: 0x%02X\n", dir);
      encontrados++;
    }
  }
  if (encontrados == 0) {
    Serial.println("  ninguno. Revisa VCC/GND y que SDA/SCL esten en los GPIO configurados.");
  } else {
    Serial.printf("Direccion esperada de la pantalla: 0x%02X\n", OLED_I2C_ADDR);
  }
}

void setcode(const std::string& arg) {
  std::string codigo;
  if (!normalizarCodigo(arg, codigo)) {
    Serial.println("Codigo invalido: deben ser exactamente 12 caracteres hexadecimales (0-9, a-f).");
    return;
  }
  if (!almacen::guardarCodigo(codigo)) {
    Serial.println("Error: no se pudo guardar el codigo en NVS.");
    return;
  }
  almacen::borrarToken();  // un token anterior pertenece al codigo viejo
  Serial.println("Codigo guardado.");
#if DEBUG_LOG_SECRETS
  Serial.printf("[debug] codigo=%s\n", codigo.c_str());
#endif
  dispositivo::reiniciar();
}

void setbackend(const std::string& arg) {
  std::string url;
  if (!validarUrlBackend(arg, url)) {
    Serial.println("URL invalida: debe empezar por http:// y tener host, p. ej. http://192.168.1.100:5001");
    return;
  }
  if (!almacen::guardarUrlBackend(url)) {
    Serial.println("Error: no se pudo guardar la URL en NVS.");
    return;
  }
  Serial.printf("Backend guardado: %s\n", url.c_str());
}

void ejecutar(const std::string& texto, const EstadoConsola& ctx) {
  std::string cmd, arg;
  parsearComando(texto, cmd, arg);
  if (cmd.empty()) return;
  if (cmd == "help") ayuda();
  else if (cmd == "status") estado(ctx);
  else if (cmd == "setcode") setcode(arg);
  else if (cmd == "setbackend") setbackend(arg);
  else if (cmd == "i2cscan") i2cscan();
  else if (cmd == "reset") dispositivo::reiniciarEmparejamiento(false);
  else if (cmd == "factory-reset") dispositivo::reiniciarEmparejamiento(true);
  else Serial.printf("Comando desconocido: %s (escribe help)\n", cmd.c_str());
}

}  // namespace

void atender(const EstadoConsola& ctx) {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (!linea.empty()) {
        const std::string completa = linea;
        linea.clear();
        ejecutar(completa, ctx);
      }
    } else if (linea.size() < 200) {
      linea += c;
    }  // las lineas mas largas se truncan
  }
}

}  // namespace consola
}  // namespace matera
