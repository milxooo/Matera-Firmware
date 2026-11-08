// Pantalla detrás de una interfaz mínima. El firmware funciona sin pantalla conectada.
#pragma once

namespace matera {

class Pantalla {
 public:
  virtual ~Pantalla() {}
  virtual void iniciar() = 0;
  // Texto de estado corto (una o dos líneas).
  virtual void mostrarEstado(const char* texto) = 0;
  // Dibuja el QR con este contenido. Solo se llama durante PROVISIONING.
  virtual void mostrarQr(const char* payload) = 0;
};

// Alternativa sin pantalla: solo escribe por Serial. NO imprime el payload del QR (lleva el código).
class PantallaSerial : public Pantalla {
 public:
  void iniciar() override;
  void mostrarEstado(const char* texto) override;
  void mostrarQr(const char* payload) override;
};

}  // namespace matera
