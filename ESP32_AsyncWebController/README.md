# ESP32_AsyncWebController v2.0.0

Professional async webserver library for ESP32 output control.

## Features

- **RESTful JSON API** - Standard HTTP endpoints for control
- **WebSocket** - Real-time bidirectional updates
- **Custom HTML Interface** - Project defines the UI via callback
- **WiFi AP & Station Mode** - Access Point or connect to existing network
- **CORS Support** - Enable cross-origin requests
- **Custom Routes** - Extend with your own endpoints

## Installation

### PlatformIO
```ini
lib_deps = 
    https://github.com/MROutake/Platform-IO-LIBS.git#ESP32_AsyncWebController
    mathieucarbou/ESPAsyncWebServer @ ^3.3.21
    mathieucarbou/AsyncTCP @ ^3.2.14
    bblanchon/ArduinoJson @ ^7.2.1
```

## Quick Start

```cpp
#include <ESP32_AsyncWebController.h>

ESP32_AsyncWebController webServer(80, 6);  // Port 80, 6 channels

// Callbacks (implement these based on your hardware)
void setOutput(uint8_t ch, bool state) {
    // Control your hardware here
}

bool getOutput(uint8_t ch) {
    // Return current state
    return false;
}

String getAllStates() {
    return "{\"channels\":{\"0\":false,\"1\":false}}";
}

String generateHTML() {
    return "<html><body><h1>My Interface</h1></body></html>";
}

void setup() {
    Serial.begin(115200);
    
    // Start Access Point
    webServer.startAP("MyDevice", "password123");
    
    // Register callbacks
    webServer.setCallbacks(setOutput, getOutput, getAllStates);
    webServer.setHTMLGenerator(generateHTML);
    
    // Start server
    webServer.begin();
}

void loop() {
    webServer.loop();  // Cleanup WebSocket clients
}
```

## API Reference

### WiFi Configuration

```cpp
bool startAP(const char* ssid, const char* password = "");
bool connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs = 10000);
String getIP() const;
```

### Callbacks

```cpp
// Control callback: (channel, state) -> void
using OutputControlCallback = std::function<void(uint8_t, bool)>;

// State callback: (channel) -> bool
using OutputStateCallback = std::function<bool(uint8_t)>;

// All states callback: () -> JSON String
using GetAllStatesCallback = std::function<String()>;

// HTML generator callback: () -> HTML String
using GetHTMLCallback = std::function<String()>;

void setCallbacks(OutputControlCallback, OutputStateCallback, GetAllStatesCallback);
void setHTMLGenerator(GetHTMLCallback);
```

### Server Configuration

```cpp
void setSystemName(const char* name);
void enableCORS(bool enable = true);
void begin();
void loop();
```

### WebSocket

```cpp
void broadcastStateChange(uint8_t channel, bool state);
```

### Custom Routes

```cpp
void addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler);
AsyncWebServer* getServer();  // For advanced configuration
```

## HTTP Endpoints

| Method | Endpoint | Description | Parameters |
|--------|----------|-------------|------------|
| GET | `/` | HTML Interface | - |
| GET | `/api/status` | Single channel state | `channel` |
| POST | `/api/output` | Set channel state | `channel`, `state` |
| GET | `/api/states` | All channel states | - |
| GET | `/api/info` | System information | - |

### Example Requests

```bash
# Get single channel status
curl "http://192.168.4.1/api/status?channel=0"

# Set channel state
curl -X POST "http://192.168.4.1/api/output?channel=0&state=1"

# Get all states
curl "http://192.168.4.1/api/states"

# Get system info
curl "http://192.168.4.1/api/info"
```

## WebSocket Protocol

Connect to `ws://192.168.4.1/ws`

### Receive Format
```json
{"channel": 0, "state": true}
```

### Send Format
```json
{"channel": 0, "state": true}
```

## FreeRTOS Best Practices

Run this library on **Core 0** (Network/WiFi core):

```cpp
void webServerTask(void* param) {
    webServer.startAP("MyDevice", "password");
    webServer.begin();
    
    for (;;) {
        webServer.loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    xTaskCreatePinnedToCore(webServerTask, "WebTask", 8192, NULL, 2, NULL, 0);
}
```

## Architecture

This library follows **separation of concerns**:

- **Library** provides: Webserver infrastructure, API endpoints, WebSocket handling
- **Project** provides: HTML/CSS/JS interface, hardware callbacks

```
┌─────────────────────────────────────────────┐
│                  Project                     │
│  ┌─────────────┐  ┌────────────────────┐   │
│  │ WebInterface│  │ Hardware Callbacks │   │
│  │   (HTML)    │  │ (setOutput, etc.)  │   │
│  └──────┬──────┘  └─────────┬──────────┘   │
│         │                    │              │
│  ┌──────┴────────────────────┴──────────┐  │
│  │      ESP32_AsyncWebController        │  │
│  │  (REST API, WebSocket, WiFi)         │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

## Dependencies

- ESPAsyncWebServer (mathieucarbou fork)
- AsyncTCP
- ArduinoJson

## Version History

- **v2.0.0** - Professional refactor, improved documentation, clean API
- **v1.0.0** - Initial release

## License

MIT License - See LICENSE file for details.
