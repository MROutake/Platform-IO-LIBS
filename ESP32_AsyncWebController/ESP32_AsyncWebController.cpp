#include "ESP32_AsyncWebController.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// ============================================================
// Konstruktor & Destruktor
// ============================================================

ESP32_AsyncWebController::ESP32_AsyncWebController(uint16_t port, uint8_t maxChannels)
  : _port(port)
  , _maxChannels(maxChannels)
  , _systemName("ESP32 Controller")
  , _corsEnabled(false)
  , _controlCallback(nullptr)
  , _stateCallback(nullptr)
  , _allStatesCallback(nullptr)
{
  _server = new AsyncWebServer(_port);
  _ws = new AsyncWebSocket("/ws");
}

ESP32_AsyncWebController::~ESP32_AsyncWebController() {
  delete _ws;
  delete _server;
}

// ============================================================
// WiFi Configuration
// ============================================================

bool ESP32_AsyncWebController::startAP(const char* ssid, const char* password) {
  WiFi.mode(WIFI_AP);
  bool success = false;
  
  if (strlen(password) > 0) {
    success = WiFi.softAP(ssid, password);
  } else {
    success = WiFi.softAP(ssid);
  }
  
  if (success) {
    Serial.printf("[WebController] AP started: %s\n", ssid);
    Serial.printf("[WebController] IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[WebController] Failed to start AP!");
  }
  
  return success;
}

bool ESP32_AsyncWebController::connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.printf("[WebController] Connecting to WiFi: %s", ssid);
  
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WebController] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    Serial.println("[WebController] Connection failed!");
    return false;
  }
}

String ESP32_AsyncWebController::getIP() const {
  if (WiFi.getMode() == WIFI_AP) {
    return WiFi.softAPIP().toString();
  } else {
    return WiFi.localIP().toString();
  }
}

// ============================================================
// Server Configuration
// ============================================================

void ESP32_AsyncWebController::setCallbacks(
  OutputControlCallback controlCallback,
  OutputStateCallback stateCallback,
  GetAllStatesCallback allStatesCallback
) {
  _controlCallback = controlCallback;
  _stateCallback = stateCallback;
  _allStatesCallback = allStatesCallback;
}

void ESP32_AsyncWebController::setSystemName(const char* name) {
  _systemName = String(name);
}

void ESP32_AsyncWebController::enableCORS(bool enable) {
  _corsEnabled = enable;
}

// ============================================================
// Server Start
// ============================================================

void ESP32_AsyncWebController::begin() {
  // WebSocket Setup
  setupWebSocket();
  _server->addHandler(_ws);
  
  // Routes Setup
  setupRoutes();
  
  // CORS Header
  if (_corsEnabled) {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  }
  
  // 404 Handler
  _server->onNotFound([this](AsyncWebServerRequest* request) {
    handleNotFound(request);
  });
  
  // Server starten
  _server->begin();
  Serial.printf("[WebController] Server started on port %d\n", _port);
  Serial.printf("[WebController] System: %s\n", _systemName.c_str());
  Serial.printf("[WebController] Channels: %d\n", _maxChannels);
}

// ============================================================
// Routes Setup
// ============================================================

void ESP32_AsyncWebController::setupRoutes() {
  // Root - HTML Interface
  _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleRoot(request);
  });
  
  // API: GET /api/status?channel=0
  _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleGetStatus(request);
  });
  
  // API: POST /api/output?channel=0&state=1
  _server->on("/api/output", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleSetOutput(request);
  });
  
  // API: GET /api/states (alle Zustände)
  _server->on("/api/states", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleGetAllStates(request);
  });
  
  // API: GET /api/info (System-Info)
  _server->on("/api/info", HTTP_GET, [this](AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["system"] = _systemName;
    doc["channels"] = _maxChannels;
    doc["ip"] = getIP();
    doc["uptime"] = millis() / 1000;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
}

// ============================================================
// WebSocket Setup
// ============================================================

void ESP32_AsyncWebController::setupWebSocket() {
  _ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, 
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    handleWebSocketEvent(server, client, type, arg, data, len);
  });
}

void ESP32_AsyncWebController::handleWebSocketEvent(
  AsyncWebSocket* server, 
  AsyncWebSocketClient* client,
  AwsEventType type, 
  void* arg, 
  uint8_t* data, 
  size_t len
) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WebSocket] Client #%u connected\n", client->id());
    
    // Sende aktuellen Status an neuen Client
    if (_allStatesCallback) {
      String states = _allStatesCallback();
      client->text(states);
    }
    
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WebSocket] Client #%u disconnected\n", client->id());
    
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      String message = String((char*)data);
      
      // Parse JSON command: {"channel": 0, "state": true}
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (!error && doc.containsKey("channel") && doc.containsKey("state")) {
        uint8_t channel = doc["channel"];
        bool state = doc["state"];
        
        if (isChannelValid(channel) && _controlCallback) {
          _controlCallback(channel, state);
          broadcastStateChange(channel, state);
        }
      }
    }
  }
}

// ============================================================
// Route Handlers
// ============================================================

void ESP32_AsyncWebController::handleRoot(AsyncWebServerRequest* request) {
  String html = generateHTML();
  request->send(200, "text/html", html);
}

void ESP32_AsyncWebController::handleGetStatus(AsyncWebServerRequest* request) {
  if (!request->hasParam("channel")) {
    request->send(400, "application/json", "{\"error\":\"Missing channel parameter\"}");
    return;
  }
  
  uint8_t channel = request->getParam("channel")->value().toInt();
  
  if (!isChannelValid(channel)) {
    request->send(400, "application/json", "{\"error\":\"Invalid channel\"}");
    return;
  }
  
  if (!_stateCallback) {
    request->send(500, "application/json", "{\"error\":\"State callback not set\"}");
    return;
  }
  
  bool state = _stateCallback(channel);
  
  JsonDocument doc;
  doc["channel"] = channel;
  doc["state"] = state;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void ESP32_AsyncWebController::handleSetOutput(AsyncWebServerRequest* request) {
  if (!request->hasParam("channel") || !request->hasParam("state")) {
    request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
    return;
  }
  
  uint8_t channel = request->getParam("channel")->value().toInt();
  bool state = request->getParam("state")->value().toInt() != 0;
  
  if (!isChannelValid(channel)) {
    request->send(400, "application/json", "{\"error\":\"Invalid channel\"}");
    return;
  }
  
  if (!_controlCallback) {
    request->send(500, "application/json", "{\"error\":\"Control callback not set\"}");
    return;
  }
  
  _controlCallback(channel, state);
  broadcastStateChange(channel, state);
  
  JsonDocument doc;
  doc["success"] = true;
  doc["channel"] = channel;
  doc["state"] = state;
  
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

void ESP32_AsyncWebController::handleGetAllStates(AsyncWebServerRequest* request) {
  if (!_allStatesCallback) {
    request->send(500, "application/json", "{\"error\":\"Callback not set\"}");
    return;
  }
  
  String states = _allStatesCallback();
  request->send(200, "application/json", states);
}

void ESP32_AsyncWebController::handleNotFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

// ============================================================
// Public Functions
// ============================================================

void ESP32_AsyncWebController::loop() {
  _ws->cleanupClients();
}

void ESP32_AsyncWebController::broadcastStateChange(uint8_t channel, bool state) {
  JsonDocument doc;
  doc["channel"] = channel;
  doc["state"] = state;
  
  String message;
  serializeJson(doc, message);
  
  _ws->textAll(message);
}

void ESP32_AsyncWebController::addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler) {
  _server->on(uri, method, handler);
}

// ============================================================
// Helper Functions
// ============================================================

bool ESP32_AsyncWebController::isChannelValid(uint8_t channel) {
  return channel < _maxChannels;
}

String ESP32_AsyncWebController::generateHTML() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + _systemName + R"(</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            text-align: center;
        }
        .info {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .channels {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        .channel {
            background: #f5f5f5;
            border-radius: 10px;
            padding: 20px;
            text-align: center;
            transition: all 0.3s;
        }
        .channel:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.1);
        }
        .channel-name {
            font-weight: bold;
            margin-bottom: 10px;
            color: #333;
        }
        .toggle-btn {
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
        }
        .toggle-btn.on {
            background: #4CAF50;
            color: white;
        }
        .toggle-btn.off {
            background: #f44336;
            color: white;
        }
        .toggle-btn:hover {
            opacity: 0.9;
            transform: scale(1.05);
        }
        .status {
            margin-top: 10px;
            font-size: 12px;
            color: #666;
        }
        .connection-status {
            text-align: center;
            padding: 10px;
            border-radius: 5px;
            margin-bottom: 20px;
            font-weight: bold;
        }
        .connected { background: #d4edda; color: #155724; }
        .disconnected { background: #f8d7da; color: #721c24; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎛️ )" + _systemName + R"(</h1>
        <div class="info">
            IP: <strong>)" + getIP() + R"(</strong> | 
            Channels: <strong>)" + String(_maxChannels) + R"(</strong>
        </div>
        <div id="wsStatus" class="connection-status disconnected">
            ⚠️ Disconnected
        </div>
        <div class="channels" id="channels"></div>
    </div>

    <script>
        const maxChannels = )" + String(_maxChannels) + R"(;
        let ws;
        let states = {};

        function initWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            
            ws.onopen = function() {
                document.getElementById('wsStatus').className = 'connection-status connected';
                document.getElementById('wsStatus').innerHTML = '✅ Connected';
            };
            
            ws.onclose = function() {
                document.getElementById('wsStatus').className = 'connection-status disconnected';
                document.getElementById('wsStatus').innerHTML = '⚠️ Disconnected - Reconnecting...';
                setTimeout(initWebSocket, 2000);
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                if (data.channels) {
                    states = data.channels;
                    updateUI();
                } else if (data.channel !== undefined) {
                    states[data.channel] = data.state;
                    updateButton(data.channel, data.state);
                }
            };
        }

        function toggleOutput(channel) {
            const newState = !states[channel];
            fetch('/api/output?channel=' + channel + '&state=' + (newState ? 1 : 0), {
                method: 'POST'
            }).then(response => response.json())
              .then(data => {
                  if (data.success) {
                      states[channel] = data.state;
                      updateButton(channel, data.state);
                  }
              });
        }

        function updateButton(channel, state) {
            const btn = document.getElementById('btn-' + channel);
            if (btn) {
                btn.className = 'toggle-btn ' + (state ? 'on' : 'off');
                btn.textContent = state ? 'ON' : 'OFF';
            }
        }

        function updateUI() {
            const container = document.getElementById('channels');
            container.innerHTML = '';
            
            for (let i = 0; i < maxChannels; i++) {
                const state = states[i] || false;
                const div = document.createElement('div');
                div.className = 'channel';
                div.innerHTML = `
                    <div class="channel-name">Channel ${i}</div>
                    <button id="btn-${i}" class="toggle-btn ${state ? 'on' : 'off'}" 
                            onclick="toggleOutput(${i})">
                        ${state ? 'ON' : 'OFF'}
                    </button>
                `;
                container.appendChild(div);
            }
        }

        // Initialize
        initWebSocket();
        fetch('/api/states')
            .then(response => response.json())
            .then(data => {
                states = data.channels || {};
                updateUI();
            });
    </script>
</body>
</html>
)";
  
  return html;
}
