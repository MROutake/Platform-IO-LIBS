/**
 * @file ESP32_AsyncWebController.h
 * @brief Professional Async Web Controller Library for ESP32
 * @version 2.0.0
 * @author MROutake
 * @date 2025
 * 
 * Industrial-grade async webserver library for controlling outputs
 * (relays, LEDs, etc.) via HTTP REST API and WebSocket.
 * 
 * Features:
 * - RESTful JSON API
 * - WebSocket for real-time updates
 * - Custom HTML interface (project-defined callback)
 * - WiFi AP or Station mode
 * - CORS support
 * - Custom route registration
 * 
 * FreeRTOS Best Practice:
 * - Run this library on Core 0 (Network/WiFi)
 * - Keep GPIO/Hardware tasks on Core 1
 * 
 * API Endpoints:
 * - GET  /              Web interface (HTML)
 * - GET  /api/status    Single channel status
 * - POST /api/output    Set channel state
 * - GET  /api/states    All channel states
 * - GET  /api/info      System information
 * - WS   /ws            WebSocket connection
 */

#ifndef ESP32_ASYNCWEBCONTROLLER_H
#define ESP32_ASYNCWEBCONTROLLER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <functional>

// ============================================================
// Version
// ============================================================
#define ASYNC_WEBCONTROLLER_VERSION "2.0.0"

// ============================================================
// Callback Types
// ============================================================

/**
 * @brief Callback to control an output channel
 * @param channel Channel number (0-based)
 * @param state Desired state (true = ON, false = OFF)
 */
using OutputControlCallback = std::function<void(uint8_t channel, bool state)>;

/**
 * @brief Callback to read single channel state
 * @param channel Channel number (0-based)
 * @return Current state (true = ON, false = OFF)
 */
using OutputStateCallback = std::function<bool(uint8_t channel)>;

/**
 * @brief Callback to get all channel states as JSON
 * @return JSON string with channel states
 * @example {"channels":{"0":true,"1":false,"2":true}}
 */
using GetAllStatesCallback = std::function<String()>;

/**
 * @brief Callback to generate HTML interface
 * @return Complete HTML document string
 */
using GetHTMLCallback = std::function<String()>;

// ============================================================
// ESP32_AsyncWebController Class
// ============================================================

/**
 * @class ESP32_AsyncWebController
 * @brief Async webserver for output control
 * 
 * This library provides the web infrastructure only.
 * The actual HTML interface is provided by the project via callback.
 * 
 * Usage Example:
 * @code
 * ESP32_AsyncWebController webServer(80, 6);
 * 
 * void setup() {
 *     webServer.startAP("MyDevice", "password123");
 *     webServer.setCallbacks(setOutput, getOutput, getAllOutputs);
 *     webServer.setHTMLGenerator(generateHTML);
 *     webServer.begin();
 * }
 * 
 * void loop() {
 *     webServer.loop();  // Clean up WebSocket clients
 * }
 * @endcode
 */
class ESP32_AsyncWebController {
public:
    // ========== Constructor / Destructor ==========
    
    /**
     * @brief Constructor
     * @param port HTTP server port (default: 80)
     * @param maxChannels Maximum number of output channels
     */
    ESP32_AsyncWebController(uint16_t port = 80, uint8_t maxChannels = 8);
    
    /**
     * @brief Destructor
     */
    ~ESP32_AsyncWebController();
    
    // ========== WiFi Configuration ==========
    
    /**
     * @brief Start WiFi Access Point
     * @param ssid SSID for the access point
     * @param password Password (empty = open network)
     * @return true if successful
     */
    bool startAP(const char* ssid, const char* password = "");
    
    /**
     * @brief Connect to existing WiFi network
     * @param ssid Network SSID
     * @param password Network password
     * @param timeoutMs Connection timeout in milliseconds
     * @return true if connected
     */
    bool connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs = 10000);
    
    /**
     * @brief Get current IP address
     * @return IP address as string
     */
    String getIP() const;
    
    // ========== Callback Configuration ==========
    
    /**
     * @brief Set output control callbacks
     * @param controlCallback Callback to set output state
     * @param stateCallback Callback to read output state
     * @param allStatesCallback Callback to read all states (JSON)
     */
    void setCallbacks(
        OutputControlCallback controlCallback,
        OutputStateCallback stateCallback,
        GetAllStatesCallback allStatesCallback
    );
    
    /**
     * @brief Set HTML generator callback
     * @param htmlCallback Function returning HTML string
     * @note The HTML interface is defined by the project, not this library
     */
    void setHTMLGenerator(GetHTMLCallback htmlCallback);
    
    // ========== Server Configuration ==========
    
    /**
     * @brief Set system name (shown in /api/info)
     * @param name System identifier
     */
    void setSystemName(const char* name);
    
    /**
     * @brief Enable CORS headers
     * @param enable true to enable CORS
     */
    void enableCORS(bool enable = true);
    
    /**
     * @brief Start the webserver
     */
    void begin();
    
    /**
     * @brief Maintenance loop (call in main loop)
     * Cleans up disconnected WebSocket clients
     */
    void loop();
    
    // ========== WebSocket Broadcasting ==========
    
    /**
     * @brief Broadcast state change to all WebSocket clients
     * @param channel Channel number
     * @param state New state
     */
    void broadcastStateChange(uint8_t channel, bool state);
    
    // ========== Custom Routes ==========
    
    /**
     * @brief Add custom route
     * @param uri URI path (e.g., "/api/custom")
     * @param method HTTP method (HTTP_GET, HTTP_POST, etc.)
     * @param handler Request handler function
     */
    void addRoute(const char* uri, WebRequestMethod method, ArRequestHandlerFunction handler);
    
    /**
     * @brief Get AsyncWebServer instance for advanced configuration
     * @return Pointer to AsyncWebServer
     */
    AsyncWebServer* getServer() { return _server; }

private:
    // Server instances
    AsyncWebServer* _server;
    AsyncWebSocket* _ws;
    
    // Configuration
    uint16_t _port;
    uint8_t _maxChannels;
    String _systemName;
    bool _corsEnabled;
    
    // Callbacks
    OutputControlCallback _controlCallback;
    OutputStateCallback _stateCallback;
    GetAllStatesCallback _allStatesCallback;
    GetHTMLCallback _htmlCallback;
    
    // Internal setup
    void setupRoutes();
    void setupWebSocket();
    void handleWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                              AwsEventType type, void* arg, uint8_t* data, size_t len);
    
    // Route handlers
    void handleRoot(AsyncWebServerRequest* request);
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleSetOutput(AsyncWebServerRequest* request);
    void handleGetAllStates(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    
    // Helper
    bool isChannelValid(uint8_t channel);
};

#endif // ESP32_ASYNCWEBCONTROLLER_H
