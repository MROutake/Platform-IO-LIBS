#ifndef ESP32_ASYNCWEBCONTROLLER_H
#define ESP32_ASYNCWEBCONTROLLER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>

/**
 * @brief Callback-Typen für die Webserver-Kommunikation
 */
using OutputControlCallback = std::function<void(uint8_t channel, bool state)>;
using OutputStateCallback = std::function<bool(uint8_t channel)>;
using GetAllStatesCallback = std::function<String()>;
using GetHTMLCallback = std::function<String()>;  // NEU: Callback für HTML

/**
 * @brief AsyncWebController - Generischer Async Webserver für Output-Steuerung
 * 
 * Diese Bibliothek bietet eine saubere API für einen AsyncWebServer,
 * der Ausgänge (Relais, LEDs, etc.) über HTTP steuern kann.
 * 
 * Features:
 * - JSON-API für RESTful Control
 * - WebSocket für Echtzeit-Updates
 * - Custom HTML-Interface (vom Projekt bereitgestellt)
 * - WiFi AP oder Station Mode
 * - CORS-Support
 */
class ESP32_AsyncWebController {
public:
  /**
   * @brief Konstruktor
   * @param port HTTP Server Port (Standard: 80)
   * @param maxChannels Maximale Anzahl der Kanäle
   */
  ESP32_AsyncWebController(uint16_t port = 80, uint8_t maxChannels = 8);
  
  /**
   * @brief Destruktor
   */
  ~ESP32_AsyncWebController();
  
  // ============================================================
  // WiFi Configuration
  // ============================================================
  
  /**
   * @brief Starte WiFi Access Point
   * @param ssid SSID des Access Points
   * @param password Passwort (leer = offen)
   * @return true wenn erfolgreich
   */
  bool startAP(const char* ssid, const char* password = "");
  
  /**
   * @brief Verbinde mit bestehendem WiFi
   * @param ssid SSID des WiFi-Netzwerks
   * @param password Passwort
   * @param timeoutMs Timeout in Millisekunden
   * @return true wenn erfolgreich
   */
  bool connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs = 10000);
  
  /**
   * @brief Hole aktuelle IP-Adresse
   * @return IP-Adresse als String
   */
  String getIP() const;
  
  // ============================================================
  // Server Configuration
  // ============================================================
  
  /**
   * @brief Setze Callbacks für Output-Steuerung
   * @param controlCallback Callback zum Setzen eines Ausgangs
   * @param stateCallback Callback zum Abfragen eines Ausgangs
   * @param allStatesCallback Callback zum Abfragen aller Ausgänge (JSON)
   */
  void setCallbacks(
    OutputControlCallback controlCallback,
    OutputStateCallback stateCallback,
    GetAllStatesCallback allStatesCallback
  );
  
  /**
   * @brief Setze HTML-Generator Callback (vom Projekt bereitgestellt)
   * @param htmlCallback Callback der HTML-String zurückgibt
   */
  void setHTMLGenerator(GetHTMLCallback htmlCallback);
  
  /**
   * @brief Setze System-Name (wird im Web-Interface angezeigt)
   * @param name System-Name
   */
  void setSystemName(const char* name);
  
  /**
   * @brief Aktiviere CORS (für externe Web-Apps)
   * @param enable true = aktivieren
   */
  void enableCORS(bool enable = true);
  
  /**
   * @brief Starte den Webserver
   */
  void begin();
  
  /**
   * @brief Update-Loop (für WebSocket-Broadcast etc.)
   * Im loop() aufrufen, falls automatische Updates gewünscht
   */
  void loop();
  
  /**
   * @brief Sende Status-Update an alle WebSocket-Clients
   * @param channel Kanal-Nummer
   * @param state Status (true/false)
   */
  void broadcastStateChange(uint8_t channel, bool state);
  
  // ============================================================
  // Custom Routes
  // ============================================================
  
  /**
   * @brief Füge eigene Route hinzu
   * @param uri URI Pfad
   * @param method HTTP Methode (HTTP_GET, HTTP_POST, etc.)
   * @param handler Request-Handler
   */
  void addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler);
  
  /**
   * @brief Zugriff auf den AsyncWebServer (für erweiterte Konfiguration)
   * @return Pointer auf AsyncWebServer
   */
  AsyncWebServer* getServer() { return _server; }

private:
  // Server-Instanzen
  AsyncWebServer* _server;
  AsyncWebSocket* _ws;
  
  // Konfiguration
  uint16_t _port;
  uint8_t _maxChannels;
  String _systemName;
  bool _corsEnabled;
  
  // Callbacks
  OutputControlCallback _controlCallback;
  OutputStateCallback _stateCallback;
  GetAllStatesCallback _allStatesCallback;
  GetHTMLCallback _htmlCallback;  // NEU: HTML-Generator Callback
  
  // Interne Funktionen
  void setupRoutes();
  void setupWebSocket();
  void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                            AwsEventType type, void* arg, uint8_t* data, size_t len);
  
  // Route-Handler
  void handleGetStatus(AsyncWebServerRequest* request);
  void handleSetOutput(AsyncWebServerRequest* request);
  void handleGetAllStates(AsyncWebServerRequest* request);
  void handleRoot(AsyncWebServerRequest* request);
  void handleNotFound(AsyncWebServerRequest* request);
  
  // Helper
  bool isChannelValid(uint8_t channel);
};

#endif // ESP32_ASYNCWEBCONTROLLER_H
