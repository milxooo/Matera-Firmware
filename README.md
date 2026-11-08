# Matera-Firmware

Firmware (C++ / Arduino IDE) de la **ESP32-S3** del proyecto Matera. Implementa el lado de la
ESP32 del flujo de vinculación con el backend (`Matera`, repositorio aparte):

1. Se identifica con su MAC y guarda el **código de emparejamiento**.
2. Recibe el WiFi del usuario por **BLE provisioning** (PoP = código).
3. Se **activa** ante el backend (`POST /dispositivos/activar`) y guarda su **token de dispositivo**.
4. Muestra un **QR** en la pantalla OLED mientras está en modo emparejamiento.
5. **Reset de emparejamiento** por botón o por Serial.

Fuera de alcance por ahora: lectura de sensores y telemetría, OTA, HTTPS, bajo consumo.

> **Estado de verificación.** El código **compila** para `esp32:esp32:esp32s3` (core 3.3.12) y la
> lógica pura pasa sus pruebas en el PC. **Nada se ha probado todavía en la placa real**: usa la
> "Guía de prueba con la placa" más abajo.

## Hardware

| Elemento | Detalle |
|---|---|
| Placa | ESP32-S3 **N16R8** (16 MB flash, 8 MB PSRAM octal) |
| Pantalla | OLED 0.96" I2C, se asume **SSD1306 128x64, dirección 0x3C** (no verificado) |
| Conexión OLED | VCC→VCC, GND→GND, **SDA→GPIO 8, SCL→GPIO 9** (no chocan con la PSRAM octal, GPIO 35 a 37) |
| Botón de reset | **BOOT (GPIO0)**, mantenido 5 s |
| Red | WiFi de **2.4 GHz** (la ESP32 no conecta a 5 GHz) |

Los pines y el tiempo del botón están en `config.example.h`, no sueltos en el código.

### Los dos puertos USB-C

La placa tiene **dos** USB-C:

- **UART** (puente USB-serie, normalmente rotulado `UART` o `COM`): es el que se usa para subir
  código y ver el Serial. Con este no hace falta ningún ajuste especial.
- **USB** (USB nativo del S3, rotulado `USB`): también puede subir código, pero para ver el Serial
  hay que activar **Tools → USB CDC On Boot → Enabled** antes de compilar y subir.

Si no sale nada en el monitor serie, lo más probable es que estés en el puerto USB nativo con
"USB CDC On Boot" desactivado (o al revés). Baudios del monitor: **115200**.

## Instalación y compilación (Arduino IDE)

1. **Core ESP32.** En *File → Preferences → Additional boards manager URLs* añade
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`. En *Boards Manager*
   instala **esp32 by Espressif Systems** versión **3.3.12** (la usada para desarrollar).
2. **Biblioteca.** En *Library Manager* instala **U8g2** versión **2.36.19** (olikraus).
   La biblioteca de QR (*QRCode* de Richard Moore, MIT) ya viene **copiada en el sketch** como
   `ricmoo_qrcode.c/.h` con su licencia: no la instales aparte (su `qrcode.h` choca con el de ESP-IDF).
   `WiFi`, `WiFiProv`, `HTTPClient`, `Preferences` y `Wire` vienen con el core.
3. **Abrir** `matera_firmware/matera_firmware.ino` (la carpeta debe llamarse igual que el `.ino`).
4. **Configuración local.** Copia `matera_firmware/config.example.h` como
   `matera_firmware/config.h` y ajusta `BACKEND_BASE_URL` (o usa `setbackend` por Serial).
   `config.h` está en `.gitignore`: no subas IPs ni secretos.
5. **Ajustes de la placa** (menú *Tools*):

   | Ajuste | Valor |
   |---|---|
   | Board | **ESP32S3 Dev Module** |
   | Flash Size | **16MB (128Mb)** |
   | PSRAM | **OPI PSRAM** |
   | Partition Scheme | **16M Flash (3MB APP/9.9MB FATFS)** (`app3M_fat9M_16MB`) |
   | USB CDC On Boot | Disabled (puerto UART) / Enabled (puerto USB nativo) |
   | Core Debug Level | **None** (ver "Seguridad") |

   El firmware ocupa ~1.3 MB; el esquema de 3 MB de aplicación sobra (el BLE provisioning pesa).

Compilación por línea de comandos (opcional):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi matera_firmware
```

## Comportamiento (máquina de estados)

| Estado | Cuándo | Qué hace |
|---|---|---|
| `SIN_CODIGO` | No hay código guardado | Imprime la MAC12 y las instrucciones; espera `setcode` |
| `PROVISIONING` | Hay código, no hay WiFi guardado | BLE provisioning (servicio `PROV_MATERA_xxxx`, PoP = código) y **QR en pantalla** |
| `CONECTANDO` | Hay WiFi guardado | Conecta; si falla reintenta cada 20 s sin borrar credenciales |
| `ACTIVANDO` | Hay WiFi y no hay token | `POST /dispositivos/activar` con reintentos |
| `OPERATIVO` | Hay WiFi y token | Estado estable. Punto de extensión: telemetría (abajo) |
| `ERROR` | 404 (código inválido), 400, respuesta inesperada o token inválido | Mensaje por Serial y pantalla; salida con `reset`, `setcode` o `factory-reset` |

Reintentos en `ACTIVANDO`: **409** (la matera aún no está vinculada desde la app) es **normal** al
principio, porque la ESP32 recibe el WiFi y se activa mientras la app todavía puede estar
vinculando. Se reintenta cada 5 s, subiendo de 5 en 5 hasta 30 s, mientras haya WiFi. Lo mismo para
5xx y errores de red. 404/400 no se reintentan.

Las peticiones HTTP bloquean el `loop` hasta `HTTP_TIMEOUT_MS` (8 s) en el peor caso; el resto del
tiempo el `loop` usa `millis()` y no hay `delay()` largos.

### Contrato con el backend

`POST {BACKEND_BASE_URL}/dispositivos/activar`, `Content-Type: application/json`,
cuerpo `{"mac":"<MAC12>","codigo":"<CODIGO>"}`. MAC12 = 12 hex en **MAYÚSCULAS** sin separadores
(`246F28AB1234`); el código se envía **exactamente** como se guardó (12 hex en **minúsculas**).

| HTTP | Acción |
|---|---|
| 200 `{"token":"<JWT>"}` | Guardar el token → `OPERATIVO` |
| 404 | Código no corresponde a la MAC → `ERROR`, sin reintento |
| 409 | Matera aún no vinculada → reintentar |
| 400 | Formato inválido (error de programación) → `ERROR` |
| 5xx / red | Reintentar con espera creciente |

### Reset de emparejamiento

- Mantener **BOOT (GPIO0) 5 s**, o el comando `reset`: borra el **WiFi** (`esp_wifi_restore()`) y el
  **token**, **conserva el código** y reinicia. Al arrancar, no haber WiFi guardado deja el estado en
  `PROVISIONING`.
- `factory-reset`: igual, y además borra el **código** (queda en `SIN_CODIGO`).
- Siempre termina en `ESP.restart()`: con `NETWORK_PROV_SCHEME_HANDLER_FREE_BLE` la memoria del BLE
  solo se recupera reiniciando. Si el WiFi recibido por BLE era incorrecto, el firmware también hace
  este reset (conservando el código) a los 3 s para volver a permitir el provisioning.

### Punto de extensión: telemetría

En estado `OPERATIVO` no se hace nada más todavía. El envío futuro de lecturas va en el `case
Estado::Operativo` del `loop()` de `matera_firmware.ino` y debe usar el header
`Authorization: Bearer <token>` con `almacen::token()`.

## Consola Serial (115200 baudios, fin de línea Newline)

| Comando | Efecto |
|---|---|
| `help` | Lista los comandos |
| `status` | Estado, MAC12, URL activa del backend, y si hay código / WiFi / token (**sin mostrar secretos**) |
| `setcode <12 hex>` | Valida (12 hex), guarda en minúsculas, borra el token anterior y **reinicia** |
| `setbackend <url>` | Debe empezar por `http://`; se guarda en NVS y **tiene prioridad** sobre `BACKEND_BASE_URL` |
| `i2cscan` | Lista las direcciones I2C encontradas (verifica la pantalla) |
| `reset` | Ver "Reset de emparejamiento" |
| `factory-reset` | Ver "Reset de emparejamiento" |

## Pantalla y QR

- La pantalla está detrás de una interfaz mínima (`Pantalla`: `mostrarEstado(texto)`, `mostrarQr(payload)`).
  Si la OLED no responde en I2C al arrancar, el firmware **sigue funcionando** y escribe por Serial.
- Payload del QR, exacto: `MAT:<MAC12>:<CODIGO>`, p. ej. `MAT:246F28AB1234:f6dd13792403`
  (29 caracteres; el código va en minúsculas). Modo byte, corrección de errores **L**, versión 2
  (25x25 módulos), 2 px por módulo y zona de silencio de 2 módulos: **58x58 px** de los 64 de alto.
- Módulos **negros sobre fondo blanco** (no invertido): se rellena la pantalla de blanco primero.
- El QR se muestra **solo en `PROVISIONING`**. En los demás estados, texto corto.
- **El QR queda pequeño** (~1 cm de lado en una pantalla de 0.96"). Para escanearlo: acerca el
  móvil (10 a 15 cm), evita reflejos y pon el brillo del móvil al máximo.
- **¿SSD1306 o SH1106?** Muchos módulos genéricos de 0.96" son SH1106. Si la imagen sale corrida
  ~2 px, con ruido en un borde o cortada, pon `#define OLED_ES_SH1106 1` en `config.h`.
  Confirma la dirección con `i2cscan` (normalmente `0x3C`; algunos módulos usan `0x3D`).

## Seguridad y limitaciones

- La ESP32 solo conoce su **código** y su **token**. Nunca pongas en el firmware ni leas del backend
  `CLAIM_MASTER_KEY`, `JWT_SECRET`, `DEVICE_JWT_SECRET` ni nada del `.env`.
- Por Serial **no** se imprimen el código, el token ni el WiFi. Solo con `#define DEBUG_LOG_SECRETS 1`
  en `config.h` (desactivado por defecto).
- La biblioteca `WiFiProv` del core escribe el PoP (el código) en su log interno si **Core Debug
  Level** es *Info* o superior. Déjalo en **None**.
- **NVS no está cifrado** por defecto: quien tenga la placa puede leer el código y el token del
  flash. Limitación aceptada para este proyecto académico.
- **HTTP plano**: solo para desarrollo en red local. **Debe pasar a HTTPS antes de usarse fuera de
  la red local** (el token viajaría en claro).

## Pruebas en el PC (sin placa)

La lógica pura (`matera_logic.h/.cpp`: validar código y URL, payload del QR, decisiones por código
HTTP, reintentos, extracción del token, botón mantenido) no incluye `Arduino.h` y se prueba con
`g++` y `assert`:

```bash
tests/run_tests.sh
```

Además comprueba que el QR de 29 bytes sale en versión 2, ECC bajo, 25x25 (cabe en 58 px).

## Guía de prueba con la placa

Para quien tiene la placa y no usa Claude Code. Necesitas: la placa conectada por el USB-C
**UART**, el repositorio del backend `Matera` con su `.venv` y `backend/.env` configurados, un móvil
con la app **ESP BLE Provisioning** (de Espressif, Android o iOS) y una red WiFi de **2.4 GHz**.

### 0. Preparación

1. Sube el firmware (sección "Instalación"), abre el *Serial Monitor* a **115200** baudios con
   "Newline" y escribe `help`. **Esperado:** la lista de comandos.
   *Si no sale nada:* revisa el puerto USB-C (ver arriba), pulsa el botón RESET/EN de la placa, y
   comprueba "USB CDC On Boot".
2. Anota la IP de tu PC en la red local (`ip -4 addr`). La llamaremos `IP_DEL_PC`.

### 1. Verificar la pantalla: `i2cscan`

3. Escribe `i2cscan`. **Esperado:**
   ```
   Escaneando I2C (SDA=GPIO8, SCL=GPIO9)...
     encontrado: 0x3C
   Direccion esperada de la pantalla: 0x3C
   ```
   *Si no encuentra nada:* revisa VCC, GND, SDA→GPIO 8, SCL→GPIO 9 (¿invertidos?). *Si encuentra otra
   dirección* (p. ej. `0x3D`): cámbiala en `OLED_I2C_ADDR` de `config.h`. Sin pantalla el firmware
   funciona igual, solo por Serial.
4. Reinicia la placa. **Esperado:** `OLED iniciada.` y la pantalla muestra texto. *Si la imagen sale
   corrida o con ruido:* pon `OLED_ES_SH1106 1` en `config.h` y vuelve a subir. Si al arrancar sale
   `OLED no responde en 0x3C...`, vuelve al paso 3.

### 2. Backend accesible desde la red

5. En el PC, desde la carpeta del repositorio `Matera`, arranca el backend **sin** `APP_ENV=development`
   (así no se expone la página `/dev/test` en tu red):
   ```bash
   ./.venv/bin/flask --app backend.app:create_app run --host 0.0.0.0 --port 5001
   ```
   (`python -m backend.app` solo escucha en 127.0.0.1: la ESP32 no lo alcanzaría.)
6. Si el firewall `ufw` está activo (`sudo ufw status`), permite la red local:
   ```bash
   sudo ufw allow from 192.168.0.0/24 to any port 5001 proto tcp
   ```
7. Desde **otro** dispositivo de la misma red (o el propio PC con `IP_DEL_PC`), comprueba:
   ```bash
   curl -i -X POST http://IP_DEL_PC:5001/dispositivos/activar -H 'Content-Type: application/json' -d '{}'
   ```
   **Esperado:** `HTTP 400` (faltan campos). *Si no conecta:* firewall o el backend sigue en 127.0.0.1.
8. En el monitor serie: `setbackend http://IP_DEL_PC:5001`. **Esperado:** `Backend guardado: http://IP_DEL_PC:5001`.
   `status` muestra esa URL.

### 3. Registrar la matera y dar el código

9. Reinicia la placa y copia la **MAC12** de la línea `MAC12: 246F28AB1234`. **Esperado además:**
   `Estado: SIN_CODIGO`.
10. En el PC (carpeta `Matera`): `./.venv/bin/python -m backend.materas.cli <MAC12>`. **Esperado:**
    `MAT:<MAC12>:<codigo>`. El **código** es lo que va después del segundo `:` (12 hex minúsculas).
    Es un secreto: no lo pegues en chats ni commits.
11. En el monitor serie: `setcode <codigo>`. **Esperado:** `Codigo guardado.`, la placa reinicia y
    aparece `Estado: PROVISIONING` y `Provisioning BLE iniciado. Servicio: PROV_MATERA_xxxx`.
    La OLED muestra el **QR** (módulos negros sobre blanco). *Si `Codigo invalido`:* deben ser
    exactamente 12 caracteres 0-9 a-f.

### 4. Dar el WiFi por BLE

12. En el móvil abre **ESP BLE Provisioning** → *Provision Device* → (si pide QR, elige *I don't have
    a QR code*) → busca por BLE y elige **`PROV_MATERA_xxxx`** (los 4 últimos hex de la MAC).
13. Cuando pida el **PoP**, escribe el **código** (12 hex minúsculas) a mano. Elige tu red de **2.4
    GHz** y su contraseña. **Esperado por Serial:** `Provisioning: credenciales WiFi recibidas.`,
    `Provisioning: exito.`, `WiFi conectado.`, `Estado: ACTIVANDO`.
    *Si falla:* `Provisioning: fallo (clave incorrecta o red no encontrada). Reiniciando...` → la
    placa reinicia sola en `PROVISIONING` (conserva el código); repite desde el paso 12. Si el móvil
    no ve el servicio BLE, acerca el móvil y reinicia la placa. Si el PoP se rechaza, el código del
    paso 11 no es el mismo que escribiste en el móvil.

### 5. Activación: 409 y reintentos

14. Ya en `ACTIVANDO`, **antes de vincular**, debe verse repetido:
    `Matera aun no vinculada desde la app (409). Reintento en 5 s.` (luego 10, 15... hasta 30 s).
    *Si ves* `Sin respuesta del backend (error -1)`: la ESP32 no alcanza el backend (revisa los pasos
    5 a 8, que PC y placa estén en la misma red, y `status`).
15. Simula la app desde el PC (el usuario ya registrado; ajusta correo/contraseña):
    ```bash
    TOKEN=$(curl -s -X POST http://127.0.0.1:5001/login -H 'Content-Type: application/json' \
      -d '{"correo":"ana@example.com","contrasena":"Clave123"}' | python3 -c 'import sys,json; print(json.load(sys.stdin)["token"])')
    curl -i -X POST http://127.0.0.1:5001/materas/vincular -H "Authorization: Bearer $TOKEN" \
      -H 'Content-Type: application/json' -d '{"mac":"<MAC12>","codigo":"<codigo>"}'
    ```
    **Esperado:** `HTTP 200` `Matera vinculada`.
16. En el siguiente reintento. **Esperado por Serial:** `Activacion correcta: token de dispositivo
    guardado.` y `Estado: OPERATIVO`. `status` debe decir `Token: si`. *Si sale `ERROR 404`:* el
    código no corresponde a la MAC (repite el paso 10/11 con `factory-reset`).

### 6. Reiniciar y probar el reset

17. Reinicia la placa (botón RESET). **Esperado:** vuelve a `CONECTANDO` → `OPERATIVO` sin
    provisioning (WiFi y token persistidos).
18. Escribe `reset`. **Esperado:** `Reset: WiFi y token borrados (el codigo se conserva).`, reinicia, y
    queda en `PROVISIONING` con el QR en pantalla. Repite con **BOOT mantenido 5 s** en `OPERATIVO`
    (**Esperado:** `Boton BOOT mantenido: reset de emparejamiento.`).
19. `factory-reset` → tras reiniciar, `Estado: SIN_CODIGO`.

Si algo no coincide con lo esperado, anota la salida completa del monitor serie (sin el código ni
el token) y el paso donde falló.

## Estructura

```
matera_firmware/
  matera_firmware.ino        máquina de estados y loop
  config.example.h           plantilla de configuración (copiar a config.h)
  matera_logic.h/.cpp        lógica pura (sin Arduino.h), probada en el PC
  matera_storage.h/.cpp      NVS (código, token, URL del backend)
  matera_device.h/.cpp       MAC, WiFi guardado, reset de emparejamiento
  matera_red.h/.cpp          eventos WiFi y BLE provisioning
  matera_activation.h/.cpp   POST /dispositivos/activar
  matera_console.h/.cpp      consola Serial
  matera_display.h/.cpp      interfaz de pantalla + alternativa solo-Serial
  matera_display_oled.h/.cpp OLED con U8g2 y QR
  ricmoo_qrcode.c/.h         copia local de QRCode (MIT, ver ricmoo_qrcode.LICENSE.txt)
tests/
  test_logic.cpp, test_qr_fit.c, run_tests.sh
```

## Problemas conocidos del entorno de compilación

- En un equipo cuyo caché de bytecode del Python del sistema esté corrupto, `arduino-cli compile` falla
  con `ValueError: bad marshal data` al generar la tabla de particiones. Solución sin tocar el sistema:
  `export PYTHONPYCACHEPREFIX=/tmp/pycache` antes de compilar. En Arduino IDE no suele ocurrir.
