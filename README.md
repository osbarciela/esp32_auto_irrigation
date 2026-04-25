# Auto Irrigation

Sistema de riego automático basado en ESP32. Controla una o varias zonas de riego mediante sensores de humedad capacitivos y relés, con lógica horaria, control remoto por MQTT y actualizaciones OTA.

## Hardware

| Componente | Modelo | Conexión |
|-----------|--------|----------|
| Microcontrolador | ESP32 Dev Module | — |
| Sensor de humedad | Capacitive Soil Moisture v2.0 | GPIO 34 (ADC1) |
| Módulo relé | 1 canal, activo HIGH | GPIO 26 |
| Bomba de agua | — | Alimentación independiente 5V/12V |

> **Importante:** la bomba debe tener su propia fuente de alimentación, completamente separada del ESP32. El relé actúa como interruptor entre ambos circuitos. Compartir el rail 5V provoca reinicios por brownout al arrancar el motor.

## Lógica de riego

El sistema riega automáticamente cuando se cumplen **todas** estas condiciones:

1. La hora actual está dentro de la ventana horaria configurada
2. La humedad del sensor está por debajo del umbral de inicio
3. El NTP está sincronizado

El riego se detiene cuando:
- La humedad sube por encima del umbral de parada (histéresis)
- La ventana horaria termina
- Se supera el timeout de seguridad (por defecto 5 minutos)

Cada zona admite **dos ventanas horarias** independientes (p.ej. amanecer y atardecer).

## Configuración inicial

Los parámetros de conexión se guardan en la NVS del ESP32 y solo se escriben la primera vez. Para cambiarlos, edita `lib/Config/Config.cpp` y borra la flash antes de flashear:

```bash
pio run --target erase
pio run --target upload
```

Parámetros a configurar en `Config.cpp`:

```cpp
_prefs.putString("wifi_ssid", "tu_red");
_prefs.putString("wifi_pass", "tu_contraseña");
_prefs.putString("mqtt_host", "192.168.1.x");
_prefs.putUShort("mqtt_port", 1883);
_prefs.putString("mqtt_user", "usuario");
_prefs.putString("mqtt_pass", "contraseña");
```

### Calibración del sensor

Con el sensor conectado al GPIO 34, sube el firmware y abre el monitor serie. Se imprime la lectura raw cada 5 segundos:

```
Sensor reading : 3282
```

Toma dos medidas:
- Sensor en el **aire** → valor para `adcDryValue`
- Sensor en **agua** → valor para `adcWetValue`

Actualiza los valores en `Config.cpp` o envíalos por MQTT tras el primer arranque.

## Control por MQTT

**Topic de comandos:** `irrigation/commands`  
**Topic de métricas:** `irrigation/metrics` (cada 30 s)  
**Topic de estado:** `irrigation/status` (LWT + anuncio de conexión)

### Comandos disponibles

**Riego manual:**
```json
{ "cmd": "zone_on",  "zone": 0 }
{ "cmd": "zone_off", "zone": 0 }
```

**Configurar ventanas y umbrales:**
```json
{
  "cmd": "set_zone_config",
  "zone": 0,
  "window_start": "07:00",
  "window_end": "08:00",
  "window2_start": "20:00",
  "window2_end": "21:00",
  "moisture_start": 40.0,
  "moisture_stop": 65.0,
  "max_pump_s": 300
}
```
Solo es necesario incluir los campos que se quieran cambiar. Los cambios se persisten en NVS y sobreviven reinicios.

**Otros:**
```json
{ "cmd": "publish_metrics" }
{ "cmd": "restart" }
{ "cmd": "ota", "url": "http://192.168.1.x:8080/firmware.bin" }
```

### Ejemplo de mensaje de métricas

```json
{
  "device": "irrigation-esp32",
  "uptime_s": 3600,
  "timestamp": "2026-04-25T08:00:00+0200",
  "wifi_rssi": -62,
  "zones": [
    {
      "id": 0,
      "name": "Zona 0",
      "state": "IRRIGATING",
      "moisture_pct": 34.2,
      "pump_on": true,
      "pump_running_ms": 12400,
      "sensor_valid": true
    }
  ]
}
```

## OTA

Sirve el binario compilado con cualquier servidor HTTP:

```bash
python -m http.server 8080 --directory .pio/build/esp32dev/
```

Luego envía el comando:

```json
{ "cmd": "ota", "url": "http://192.168.1.x:8080/firmware.bin" }
```

El ESP32 para todas las bombas, descarga el firmware y se reinicia automáticamente.

## Añadir más zonas de riego

1. Conecta el nuevo sensor a un GPIO ADC1 (32–39) y el relé a cualquier GPIO de salida.
2. Incrementa `zone_count` en NVS o actualiza el valor por defecto en `Config.cpp`.
3. Añade la configuración de la nueva zona con `setZoneConfig(n, cfg)`.
4. Reflashea — `IrrigationController` construye todas las zonas automáticamente desde la config.
