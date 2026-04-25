# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build commands

```bash
pio run                        # compile
pio run --target upload        # compile + flash
pio run --target erase         # erase entire flash (clears NVS config)
pio device monitor             # serial monitor at 115200
pio run --target upload && pio device monitor   # flash + monitor in one step
```

## Architecture

PlatformIO/Arduino project for an ESP32 automatic irrigation controller. All business logic lives under `lib/` as independent PlatformIO libraries; `src/main.cpp` only wires them together in `setup()` / `loop()`.

### Library layers

| Library | Responsibility |
|---------|---------------|
| `lib/Config` | Wraps ESP32 `Preferences` (NVS). Single source of truth for all persistent state. `ZoneConfig` struct holds all per-zone parameters. Getters are `const` — `_prefs` is `mutable` to work around the non-const Preferences API. |
| `lib/Hardware` | `MoistureSensor` — ADC1 only (ADC2 is blocked when WiFi is active), 10-sample oversampling, maps raw → %. `RelayDriver` — abstracts active-HIGH vs active-LOW relays. |
| `lib/Connectivity` | `WiFiManager` — non-blocking state machine, fires `onConnected`/`onDisconnected` callbacks. `NtpSync` — `configTime()` + Spain TZ (`CET-1CEST,M3.5.0,M10.5.0/3`), exposes `isInsideWindow()`. `MqttClient` — wraps PubSubClient; uses a static instance pointer for the C-style callback; resubscribes all topics on reconnect. |
| `lib/Irrigation` | `IrrigationZone` — state machine (IDLE → IRRIGATING → TIMEOUT_LOCKOUT; MANUAL_ON bypass). Each zone has two configurable time windows (window1 always active, window2 optional). `IrrigationController` — factory that builds zones from `Config`; owns sensors/relays/zones via `unique_ptr`. |
| `lib/Telemetry` | Publishes JSON to `irrigation/metrics` on a configurable interval. Receives a `PublishFn` lambda so it has no direct dependency on `MqttClient`. |
| `lib/OtaUpdater` | `triggerUpdate()` sets a flag; actual `HTTPUpdate` runs on the next `loop()` tick (outside the MQTT callback) after stopping all pumps. |

### Data flow

```
WiFiManager ──onConnected──► NtpSync::begin() + MqttClient::reconnect()
MqttClient  ──onCommand───► onMqttCommand() in main.cpp
                               ├─ IrrigationController (zone on/off, config)
                               └─ OtaUpdater::triggerUpdate()
loop(): wifi → mqtt → ntp → IrrigationController::update() → Telemetry → OtaUpdater
```

### Config persistence

NVS keys use short prefixes to stay under the 15-char NVS key limit (`z0_sp`, `z0_mst`, `z0_w2sh`, …). Config is written on first boot when `initialized` key is absent. To reset to defaults: `pio run --target erase`.

### Adding a new irrigation zone

1. Call `config.setZoneCount(n)` and `config.setZoneConfig(n-1, cfg)` once (or erase + update defaults in `Config.cpp`).
2. Wire the new sensor to an ADC1 GPIO (32–39) and the relay to any output GPIO.
3. `IrrigationController::begin()` rebuilds all zones from config automatically.

## MQTT interface

**Subscribe:** `irrigation/commands`  
**Publish:** `irrigation/metrics` (every 30 s), `irrigation/status` (LWT + online announcement)

Key commands:

```json
{ "cmd": "zone_on",  "zone": 0 }
{ "cmd": "zone_off", "zone": 0 }
{ "cmd": "set_zone_config", "zone": 0,
  "moisture_start": 40.0, "moisture_stop": 65.0,
  "window_start": "06:00", "window_end": "08:00",
  "window2_start": "20:00", "window2_end": "21:00",
  "window2_enabled": true,
  "max_pump_s": 300 }
{ "cmd": "ota", "url": "http://192.168.1.x:8080/firmware.bin" }
{ "cmd": "publish_metrics" }
{ "cmd": "restart" }
```

## Hardware notes

- **Sensor:** Capacitive Soil Moisture v2.0 on GPIO 34. Calibration values in NVS: `adcDryValue` ≈ 3330 (air), `adcWetValue` ≈ 1040 (water).
- **Relay:** GPIO 26, `relayActiveLow = false` (activates on HIGH).
- **Pump power supply must be independent from the ESP32.** Sharing the 5V rail causes brownout resets when the motor starts.
- C++ standard is `gnu++17` (`build_unflags = -std=gnu++11` required in `platformio.ini`).
