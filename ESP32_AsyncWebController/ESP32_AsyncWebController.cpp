/**
 * @file ESP32_AsyncWebController.cpp
 * @brief Professional Async Web Controller Implementation
 * @version 2.0.0
 */

#include "ESP32_AsyncWebController.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// ============================================================
// Constructor / Destructor
// ============================================================

ESP32_AsyncWebController::ESP32_AsyncWebController(uint16_t port, uint8_t maxChannels)
    : _port(port)
    , _maxChannels(maxChannels)
    , _systemName("ESP32 Controller")
    , _corsEnabled(false)
    , _controlCallback(nullptr)
    , _stateCallback(nullptr)
    , _allStatesCallback(nullptr)
    , _htmlCallback(nullptr)
{
    _server = new AsyncWebServer(_port);
    _ws = new AsyncWebSocket("/ws");
}

ESP32_AsyncWebController::~ESP32_AsyncWebController() {
    if (_ws != nullptr) {
        delete _ws;
        _ws = nullptr;
    }
    if (_server != nullptr) {
        delete _server;
        _server = nullptr;
    }
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
        Serial.printf("[WebController] IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WebController] ERROR: Failed to start AP!");
    }
    
    return success;
}

bool ESP32_AsyncWebController::connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.printf("[WebController] Connecting to: %s", ssid);
    
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WebController] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    
    Serial.println("[WebController] ERROR: Connection failed!");
    return false;
}

String ESP32_AsyncWebController::getIP() const {
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

// ============================================================
// Callback Configuration
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

void ESP32_AsyncWebController::setHTMLGenerator(GetHTMLCallback htmlCallback) {
    _htmlCallback = htmlCallback;
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
    // WebSocket setup
    setupWebSocket();
    _server->addHandler(_ws);
    
    // Routes setup
    setupRoutes();
    
    // CORS headers
    if (_corsEnabled) {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    }
    
    // 404 handler
    _server->onNotFound([this](AsyncWebServerRequest* request) {
        handleNotFound(request);
    });
    
    // Start server
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
      
      if (!error && doc["channel"].is<uint8_t>() && doc["state"].is<bool>()) {
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
  if (_htmlCallback) {
    // HTML vom Projekt abrufen
    String html = _htmlCallback();
    request->send(200, "text/html", html);
  } else {
    // Fallback wenn kein HTML-Generator gesetzt
    request->send(500, "text/plain", "No HTML generator configured. Use setHTMLGenerator() to provide custom HTML.");
  }
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
