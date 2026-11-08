// Implementación de Pantalla para OLED I2C 128x64 con U8g2 (SSD1306 o SH1106, ver config.h).
// Si la pantalla no responde en I2C, el firmware sigue funcionando y solo escribe por Serial.
#pragma once

#include "matera_display.h"

namespace matera {

class PantallaOled : public Pantalla {
 public:
  void iniciar() override;
  void mostrarEstado(const char* texto) override;
  void mostrarQr(const char* payload) override;
  bool presente() const { return presente_; }

 private:
  bool presente_ = false;
  PantallaSerial alternativa_;  // se usa si no hay pantalla
};

}  // namespace matera
