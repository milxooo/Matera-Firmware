#include "matera_display_oled.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "ricmoo_qrcode.h"  // copia local de la biblioteca QRCode (ver README): <qrcode.h> choca con el de ESP-IDF

#include <string>

#include "config.h"
#include "matera_logic.h"

namespace matera {

namespace {

#if OLED_ES_SH1106
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_I2C_SCL, PIN_I2C_SDA);
#endif

constexpr int PX_POR_MODULO = 2;
constexpr int MODULOS_SILENCIO = 2;  // zona de silencio alrededor del QR

}  // namespace

void PantallaOled::iniciar() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.beginTransmission(OLED_I2C_ADDR);
  presente_ = (Wire.endTransmission() == 0);  // sondeo: si no responde, no se toca la pantalla
  if (!presente_) {
    Serial.printf("OLED no responde en 0x%02X (SDA=GPIO%d, SCL=GPIO%d): sigo sin pantalla. Prueba 'i2cscan'.\n",
                  OLED_I2C_ADDR, PIN_I2C_SDA, PIN_I2C_SCL);
    alternativa_.iniciar();
    return;
  }
  u8g2.setI2CAddress(OLED_I2C_ADDR << 1);  // U8g2 espera la direccion de 8 bits
  u8g2.begin();
  u8g2.setContrast(255);
  Serial.println("OLED iniciada.");
}

void PantallaOled::mostrarEstado(const char* texto) {
  if (!presente_) return alternativa_.mostrarEstado(texto);
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_6x10_tf);
  // Hasta 5 lineas separadas por '\n'.
  const std::string t(texto);
  size_t ini = 0;
  for (int linea = 0; linea < 5 && ini <= t.size(); linea++) {
    size_t fin = t.find('\n', ini);
    if (fin == std::string::npos) fin = t.size();
    u8g2.drawStr(2, 12 + linea * 12, t.substr(ini, fin - ini).c_str());
    ini = fin + 1;
  }
  u8g2.sendBuffer();
}

void PantallaOled::mostrarQr(const char* payload) {
  if (!presente_) return alternativa_.mostrarQr(payload);
  // La biblioteca QRCode no valida capacidad: se comprueba antes (v2, ECC L, modo byte = 32 bytes).
  if (!payloadQrCabe(payload)) {
    Serial.println("QR: el contenido no cabe en version 2; no se dibuja.");
    mostrarEstado("Error QR");
    return;
  }
  QRCode qr;
  uint8_t modulos[128];  // qrcode_getBufferSize(2) = 79 bytes
  if (qrcode_initText(&qr, modulos, 2, ECC_LOW, payload) != 0) {
    Serial.println("QR: no se pudo generar.");
    mostrarEstado("Error QR");
    return;
  }
  const int lado = (qr.size + 2 * MODULOS_SILENCIO) * PX_POR_MODULO;  // 58 px
  const int ox = 3;                                  // margen izquierdo
  const int oy = (OLED_ALTO - lado) / 2;             // centrado vertical
  u8g2.clearBuffer();
  // Fondo BLANCO (pixeles encendidos) y modulos NEGROS (apagados): QR no invertido.
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, OLED_ANCHO, OLED_ALTO);
  u8g2.setDrawColor(0);
  for (int y = 0; y < qr.size; y++) {
    for (int x = 0; x < qr.size; x++) {
      if (qrcode_getModule(&qr, x, y)) {
        u8g2.drawBox(ox + (MODULOS_SILENCIO + x) * PX_POR_MODULO, oy + (MODULOS_SILENCIO + y) * PX_POR_MODULO,
                     PX_POR_MODULO, PX_POR_MODULO);
      }
    }
  }
  // Texto breve a la derecha (en negro sobre blanco).
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(ox + lado + 3, 14, "Escanea");
  u8g2.drawStr(ox + lado + 3, 24, "este QR");
  u8g2.drawStr(ox + lado + 3, 34, "con la app");
  u8g2.sendBuffer();
}

}  // namespace matera
