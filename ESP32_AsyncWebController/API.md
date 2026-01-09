# ESP32 AsyncWebController - API Dokumentation

## Inhaltsverzeichnis
- [Übersicht](#übersicht)
- [Konstruktor](#konstruktor)
- [WiFi-Methoden](#wifi-methoden)
- [Konfiguration](#konfiguration)
- [Server-Steuerung](#server-steuerung)
- [REST API Endpunkte](#rest-api-endpunkte)
- [WebSocket Protokoll](#websocket-protokoll)
- [Callbacks](#callbacks)
- [Erweiterte Nutzung](#erweiterte-nutzung)

---

## Übersicht

Die `ESP32_AsyncWebController` Bibliothek bietet eine saubere, callback-basierte API für die Steuerung von ESP32-Ausgängen über einen AsyncWebServer.

### Hauptmerkmale
- ✅ RESTful JSON API
- ✅ WebSocket für Echtzeit-Updates
- ✅ Responsive HTML Web-Interface
- ✅ Thread-Safe Callbacks
- ✅ CORS Support
- ✅ Custom Routes

---

## Konstruktor

### `ESP32_AsyncWebController(uint16_t port = 80, uint8_t maxChannels = 8)`

Erstellt eine neue Instanz des WebControllers.

**Parameter:**
- `port` - HTTP Server Port (Standard: 80)
- `maxChannels` - Maximale Anzahl der steuerbaren Kanäle (Standard: 8)

**Beispiel:**
```cpp
// Standard: Port 80, 8 Kanäle
ESP32_AsyncWebController webServer(80, 8);

// Custom: Port 8080, 16 Kanäle
ESP32_AsyncWebController webServer(8080, 16);
```

---

## WiFi-Methoden

### `bool startAP(const char* ssid, const char* password = "")`

Startet einen WiFi Access Point.

**Parameter:**
- `ssid` - SSID des Access Points
- `password` - Passwort (leer = offenes Netzwerk, min. 8 Zeichen)

**Rückgabe:**
- `true` bei Erfolg
- `false` bei Fehler

**Beispiel:**
```cpp
// Offenes Netzwerk
webServer.startAP("ESP32-Controller");

// Geschütztes Netzwerk
webServer.startAP("ESP32-Controller", "12345678");
```

---

### `bool connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs = 10000)`

Verbindet mit einem bestehenden WiFi-Netzwerk.

**Parameter:**
- `ssid` - SSID des WiFi-Netzwerks
- `password` - Passwort
- `timeoutMs` - Timeout in Millisekunden (Standard: 10000)

**Rückgabe:**
- `true` bei erfolgreicher Verbindung
- `false` bei Fehler oder Timeout

**Beispiel:**
```cpp
if (webServer.connectWiFi("MyHomeWiFi", "password123", 15000)) {
  Serial.println("Connected!");
} else {
  Serial.println("Connection failed!");
}
```

---

### `String getIP() const`

Gibt die aktuelle IP-Adresse zurück.

**Rückgabe:**
- IP-Adresse als String (AP oder Station Mode)

**Beispiel:**
```cpp
String ip = webServer.getIP();
Serial.println("IP: " + ip);
```

---

## Konfiguration

### `void setCallbacks(...)`

Registriert die Callback-Funktionen für Output-Steuerung.

**Signatur:**
```cpp
void setCallbacks(
  OutputControlCallback controlCallback,
  OutputStateCallback stateCallback,
  GetAllStatesCallback allStatesCallback
)
```

**Callback-Typen:**
```cpp
// Ausgang setzen
using OutputControlCallback = std::function<void(uint8_t channel, bool state)>;

// Ausgang abfragen
using OutputStateCallback = std::function<bool(uint8_t channel)>;

// Alle Zustände als JSON
using GetAllStatesCallback = std::function<String()>;
```

**Beispiel:**
```cpp
void setOutput(uint8_t channel, bool state) {
  // Implementation
}

bool getOutput(uint8_t channel) {
  // Implementation
  return false;
}

String getAllStates() {
  return "{\"channels\":{\"0\":false,\"1\":true}}";
}

webServer.setCallbacks(setOutput, getOutput, getAllStates);
```

---

### `void setSystemName(const char* name)`

Setzt den System-Namen für das Web-Interface.

**Parameter:**
- `name` - System-Name (wird im HTML angezeigt)

**Beispiel:**
```cpp
webServer.setSystemName("My Custom Relay Board");
```

---

### `void enableCORS(bool enable = true)`

Aktiviert/Deaktiviert CORS (Cross-Origin Resource Sharing).

**Parameter:**
- `enable` - true = aktivieren, false = deaktivieren

**Beispiel:**
```cpp
// Für externe Web-Apps aktivieren
webServer.enableCORS(true);
```

---

## Server-Steuerung

### `void begin()`

Startet den Webserver.

**Hinweis:** Muss nach allen Konfigurationen aufgerufen werden!

**Beispiel:**
```cpp
webServer.startAP("ESP32");
webServer.setCallbacks(setOut, getOut, getAllOut);
webServer.begin(); // Server starten
```

---

### `void loop()`

Update-Funktion für WebSocket-Cleanup.

**Hinweis:** Im `loop()` aufrufen für optimale Performance.

**Beispiel:**
```cpp
void loop() {
  webServer.loop();
  delay(10);
}
```

---

### `void broadcastStateChange(uint8_t channel, bool state)`

Sendet ein Status-Update an alle verbundenen WebSocket-Clients.

**Parameter:**
- `channel` - Kanal-Nummer
- `state` - Neuer Status

**Beispiel:**
```cpp
void setOutput(uint8_t channel, bool state) {
  digitalWrite(pins[channel], state);
  webServer.broadcastStateChange(channel, state);
}
```

---

### `void addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler)`

Fügt eine benutzerdefinierte Route hinzu.

**Parameter:**
- `uri` - URI-Pfad (z.B. "/api/custom")
- `method` - HTTP-Methode (HTTP_GET, HTTP_POST, etc.)
- `handler` - Request-Handler Funktion

**Beispiel:**
```cpp
webServer.addRoute("/api/reset", HTTP_POST, [](AsyncWebServerRequest* req) {
  resetAllOutputs();
  req->send(200, "application/json", "{\"success\":true}");
});
```

---

### `AsyncWebServer* getServer()`

Gibt direkten Zugriff auf den AsyncWebServer.

**Rückgabe:**
- Pointer auf AsyncWebServer-Instanz

**Beispiel:**
```cpp
AsyncWebServer* server = webServer.getServer();
server->on("/advanced", HTTP_GET, [](AsyncWebServerRequest* req) {
  // Erweiterte Konfiguration
});
```

---

## REST API Endpunkte

### GET `/`

Web-Interface (HTML)

**Response:** HTML-Seite mit interaktivem Interface

---

### GET `/api/info`

System-Informationen

**Response:**
```json
{
  "system": "ESP32 Controller",
  "channels": 8,
  "ip": "192.168.4.1",
  "uptime": 12345
}
```

---

### GET `/api/states`

Alle Kanal-Zustände

**Response:**
```json
{
  "channels": {
    "0": false,
    "1": true,
    "2": false,
    "3": true
  }
}
```

---

### GET `/api/status`

Status eines einzelnen Kanals

**Query-Parameter:**
- `channel` - Kanal-Nummer (0-N)

**Request:**
```
GET /api/status?channel=2
```

**Response:**
```json
{
  "channel": 2,
  "state": false
}
```

**Fehler:**
```json
{
  "error": "Invalid channel"
}
```

---

### POST `/api/output`

Setze Kanal-Status

**Query-Parameter:**
- `channel` - Kanal-Nummer
- `state` - Status (0 oder 1)

**Request:**
```
POST /api/output?channel=3&state=1
```

**Response:**
```json
{
  "success": true,
  "channel": 3,
  "state": true
}
```

**Fehler:**
```json
{
  "error": "Missing parameters"
}
```

---

## WebSocket Protokoll

### Endpunkt: `ws://IP/ws`

### Client → Server

Sende Kommando zum Setzen eines Ausgangs:

```json
{
  "channel": 0,
  "state": true
}
```

### Server → Client

Empfange Status-Updates:

**Einzelner Kanal:**
```json
{
  "channel": 2,
  "state": false
}
```

**Alle Kanäle (bei Verbindung):**
```json
{
  "channels": {
    "0": false,
    "1": true,
    "2": false
  }
}
```

### JavaScript Beispiel

```javascript
const ws = new WebSocket('ws://192.168.4.1/ws');

ws.onopen = () => {
  console.log('Connected');
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  console.log('Update:', data);
};

// Kanal 3 einschalten
ws.send(JSON.stringify({
  channel: 3,
  state: true
}));
```

---

## Callbacks

### OutputControlCallback

**Signatur:**
```cpp
void callback(uint8_t channel, bool state)
```

**Zweck:** Wird aufgerufen, wenn ein Ausgang gesetzt werden soll.

**Beispiel:**
```cpp
void setOutput(uint8_t channel, bool state) {
  if (channel < 8) {
    digitalWrite(pins[channel], state ? HIGH : LOW);
    Serial.printf("Channel %d -> %s\n", channel, state ? "ON" : "OFF");
  }
}
```

---

### OutputStateCallback

**Signatur:**
```cpp
bool callback(uint8_t channel)
```

**Zweck:** Wird aufgerufen, wenn der Status eines Ausgangs abgefragt wird.

**Beispiel:**
```cpp
bool getOutput(uint8_t channel) {
  if (channel < 8) {
    return digitalRead(pins[channel]) == HIGH;
  }
  return false;
}
```

---

### GetAllStatesCallback

**Signatur:**
```cpp
String callback()
```

**Zweck:** Gibt alle Kanal-Zustände als JSON zurück.

**Format:**
```json
{
  "channels": {
    "0": false,
    "1": true,
    "2": false
  }
}
```

**Beispiel:**
```cpp
String getAllStates() {
  String json = "{\"channels\":{";
  for (int i = 0; i < 8; i++) {
    json += "\"" + String(i) + "\":" + (states[i] ? "true" : "false");
    if (i < 7) json += ",";
  }
  json += "}}";
  return json;
}
```

---

## Erweiterte Nutzung

### Thread-Safety mit FreeRTOS

```cpp
SemaphoreHandle_t mutex;

void setOutput(uint8_t channel, bool state) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Thread-safe operation
    controller.setOutput(channel, state);
    xSemaphoreGive(mutex);
  }
}

void setup() {
  mutex = xSemaphoreCreateMutex();
  webServer.setCallbacks(setOutput, getOutput, getAllStates);
}
```

---

### Custom Middleware

```cpp
AsyncWebServer* server = webServer.getServer();

// Logging Middleware
server->on("/*", HTTP_ANY, [](AsyncWebServerRequest* req) {
  Serial.printf("[%s] %s\n", req->methodToString(), req->url().c_str());
});
```

---

### Authentication

```cpp
webServer.addRoute("/api/secure", HTTP_GET, [](AsyncWebServerRequest* req) {
  if (!req->authenticate("admin", "password")) {
    return req->requestAuthentication();
  }
  req->send(200, "text/plain", "Authenticated!");
});
```

---

### Custom HTML Template

```cpp
String customHTML = R"(
<!DOCTYPE html>
<html>
<head><title>Custom</title></head>
<body><h1>My Custom Interface</h1></body>
</html>
)";

webServer.addRoute("/custom", HTTP_GET, [&](AsyncWebServerRequest* req) {
  req->send(200, "text/html", customHTML);
});
```

---

## Fehlerbehandlung

### Callback nicht gesetzt

```json
{
  "error": "Control callback not set"
}
```

**Lösung:** `setCallbacks()` vor `begin()` aufrufen.

---

### Ungültiger Kanal

```json
{
  "error": "Invalid channel"
}
```

**Lösung:** Kanal-Nummer muss < `maxChannels` sein.

---

### Mutex Timeout (Multi-Threading)

```cpp
if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
  // OK
  xSemaphoreGive(mutex);
} else {
  Serial.println("ERROR: Mutex timeout!");
}
```

---

## Vollständiges Beispiel

```cpp
#include <ESP32_AsyncWebController.h>

const uint8_t PINS[] = {2, 4, 5, 12};
bool states[4] = {false};

ESP32_AsyncWebController web(80, 4);

void setOut(uint8_t ch, bool state) {
  states[ch] = state;
  digitalWrite(PINS[ch], state);
}

bool getOut(uint8_t ch) {
  return states[ch];
}

String getAll() {
  String json = "{\"channels\":{";
  for (int i = 0; i < 4; i++) {
    json += "\"" + String(i) + "\":" + (states[i] ? "true" : "false");
    if (i < 3) json += ",";
  }
  json += "}}";
  return json;
}

void setup() {
  Serial.begin(115200);
  
  for (auto pin : PINS) {
    pinMode(pin, OUTPUT);
  }
  
  web.startAP("ESP32-Demo");
  web.setSystemName("4-Channel Demo");
  web.setCallbacks(setOut, getOut, getAll);
  web.begin();
}

void loop() {
  web.loop();
  delay(10);
}
```

---

## Lizenz

MIT License
