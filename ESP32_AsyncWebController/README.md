# ESP32 AsyncWebController

Eine saubere, generische Bibliothek für die Steuerung von ESP32-Ausgängen über einen AsyncWebServer mit klarer API.

## ✨ Features

- **Clean API**: Einfache Integration über Callbacks
- **RESTful JSON API**: Standardisierte HTTP-Endpunkte
- **WebSocket Support**: Echtzeit-Updates ohne Polling
- **Responsive Web-UI**: Modernes HTML-Interface inkludiert
- **WiFi Flexibilität**: AP oder Station Mode
- **CORS Support**: Für externe Web-Apps
- **Framework-agnostisch**: Funktioniert mit jeder Output-Logik

## 📦 Installation

### PlatformIO

```ini
lib_deps = 
    https://github.com/mathieucarbou/ESPAsyncWebServer.git#v3.3.21
    https://github.com/mathieucarbou/AsyncTCP.git#v3.2.14
    bblanchon/ArduinoJson@^7.0.0
```

Kopiere die Library in dein `lib/` Verzeichnis oder nutze einen Symlink.

## 🚀 Schnellstart

```cpp
#include <Arduino.h>
#include <ESP32_AsyncWebController.h>

// Webserver erstellen (Port 80, 8 Kanäle)
ESP32_AsyncWebController web(80, 8);

// Beispiel: GPIO-Array für Ausgänge
const uint8_t outputs[8] = {2, 4, 5, 12, 13, 14, 15, 16};
bool outputStates[8] = {false};

// Callback: Ausgang setzen
void setOutput(uint8_t channel, bool state) {
  if (channel < 8) {
    outputStates[channel] = state;
    digitalWrite(outputs[channel], state ? HIGH : LOW);
    Serial.printf("Output %d -> %s\n", channel, state ? "ON" : "OFF");
  }
}

// Callback: Ausgang abfragen
bool getOutput(uint8_t channel) {
  return channel < 8 ? outputStates[channel] : false;
}

// Callback: Alle Zustände (JSON)
String getAllStates() {
  String json = "{\"channels\":{";
  for (int i = 0; i < 8; i++) {
    json += "\"" + String(i) + "\":" + (outputStates[i] ? "true" : "false");
    if (i < 7) json += ",";
  }
  json += "}}";
  return json;
}

void setup() {
  Serial.begin(115200);
  
  // GPIOs initialisieren
  for (int i = 0; i < 8; i++) {
    pinMode(outputs[i], OUTPUT);
    digitalWrite(outputs[i], LOW);
  }
  
  // WiFi Access Point starten
  web.startAP("ESP32-Control", "12345678");
  
  // System-Name setzen
  web.setSystemName("My ESP32 Controller");
  
  // CORS aktivieren (optional)
  web.enableCORS(true);
  
  // Callbacks registrieren
  web.setCallbacks(setOutput, getOutput, getAllStates);
  
  // Server starten
  web.begin();
  
  Serial.println("Setup complete!");
  Serial.println("Open: http://" + web.getIP());
}

void loop() {
  web.loop(); // WebSocket Cleanup
  delay(10);
}
```

## 📡 API Endpunkte

### REST API

| Endpunkt | Methode | Parameter | Beschreibung |
|----------|---------|-----------|--------------|
| `/` | GET | - | Web-Interface (HTML) |
| `/api/info` | GET | - | System-Informationen |
| `/api/states` | GET | - | Alle Kanal-Zustände (JSON) |
| `/api/status` | GET | `channel` | Status eines Kanals |
| `/api/output` | POST | `channel`, `state` | Kanal setzen |

### Beispiel-Requests

```bash
# Status aller Kanäle
curl http://192.168.4.1/api/states

# Status von Kanal 2
curl http://192.168.4.1/api/status?channel=2

# Kanal 3 einschalten
curl -X POST http://192.168.4.1/api/output?channel=3&state=1

# System-Info
curl http://192.168.4.1/api/info
```

### WebSocket

**Endpunkt**: `ws://IP/ws`

**Empfangen** (Server → Client):
```json
{"channel": 0, "state": true}
```

**Senden** (Client → Server):
```json
{"channel": 0, "state": false}
```

## 🎯 Integration mit LatchController

```cpp
#include <LatchController.h>
#include <ESP32_AsyncWebController.h>
#include <drivers/ShiftRegisterDriver.h>

ShiftRegisterDriver relayDriver(23, 18, 19);
LatchController relays(&relayDriver, 8);
ESP32_AsyncWebController web(80, 8);

void setRelay(uint8_t channel, bool state) {
  if (state) {
    relays.setLatchOn(channel);
  } else {
    relays.setLatchOff(channel);
  }
}

bool getRelay(uint8_t channel) {
  return relays.getLatchState(channel);
}

String getAllRelays() {
  String json = "{\"channels\":{";
  for (int i = 0; i < 8; i++) {
    json += "\"" + String(i) + "\":" + (relays.getLatchState(i) ? "true" : "false");
    if (i < 7) json += ",";
  }
  json += "}}";
  return json;
}

void setup() {
  Serial.begin(115200);
  
  // Relais initialisieren
  relays.begin(ACTIVE_LOW);
  
  // WiFi starten
  web.startAP("ESP32-Relays");
  web.setSystemName("8-Channel Relay Board");
  web.setCallbacks(setRelay, getRelay, getAllRelays);
  web.begin();
}

void loop() {
  web.loop();
}
```

## 🔧 API Referenz

### Konstruktor
```cpp
ESP32_AsyncWebController(uint16_t port = 80, uint8_t maxChannels = 8);
```

### WiFi Methoden
```cpp
bool startAP(const char* ssid, const char* password = "");
bool connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs = 10000);
String getIP() const;
```

### Konfiguration
```cpp
void setCallbacks(OutputControlCallback control, OutputStateCallback state, GetAllStatesCallback allStates);
void setSystemName(const char* name);
void enableCORS(bool enable = true);
```

### Server-Steuerung
```cpp
void begin();
void loop();
void broadcastStateChange(uint8_t channel, bool state);
void addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler);
AsyncWebServer* getServer();
```

## 🎨 Anpassung

### Eigene Routes hinzufügen
```cpp
web.addRoute("/custom", HTTP_GET, [](AsyncWebServerRequest* request) {
  request->send(200, "text/plain", "Custom route!");
});
```

### Direkter Server-Zugriff
```cpp
AsyncWebServer* server = web.getServer();
server->on("/advanced", HTTP_GET, [](AsyncWebServerRequest* request) {
  // Erweiterte Konfiguration
});
```

## 📝 Lizenz

MIT License

## 🤝 Contributing

Pull Requests sind willkommen! Für größere Änderungen bitte zuerst ein Issue öffnen.

## 📧 Support

Bei Fragen oder Problemen öffne ein Issue auf GitHub.
