// Copia este archivo como config.h (queda fuera de git) y ajusta los valores.
// No pongas secretos aquí: el firmware solo conoce su código y su token.
#pragma once

// --- Backend (solo HTTP plano, solo para desarrollo en red local) ---
// IP de ejemplo: cámbiala por la IP de tu PC en config.h. También se puede cambiar
// sin recompilar con el comando Serial `setbackend <url>`, que tiene prioridad.
#define BACKEND_BASE_URL "http://192.168.1.100:5001"
#define RUTA_ACTIVAR "/dispositivos/activar"

// --- Tiempos (ms) ---
#define WIFI_CONNECT_TIMEOUT_MS 20000  // espera por intento de conexión WiFi
#define HTTP_TIMEOUT_MS 8000           // timeout de cada petición al backend

// --- Pantalla OLED I2C (0.96", se asume SSD1306 128x64) ---
// Usa el comando Serial `i2cscan` para confirmar la dirección. Si la imagen sale
// corrida o con ruido, el módulo probablemente es SH1106 (ver README).
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9
#define OLED_I2C_ADDR 0x3C
#define OLED_ANCHO 128
#define OLED_ALTO 64

// --- Reset de emparejamiento ---
#define PIN_BOTON_RESET 0       // BOOT (GPIO0); activo en nivel bajo
#define BOTON_RESET_MS 5000     // tiempo que hay que mantenerlo pulsado

// --- Depuración ---
// 1 = imprime código, token y credenciales por Serial. Dejar en 0 salvo para depurar.
#define DEBUG_LOG_SECRETS 0
