/**
 * @file BasicExample.ino
 * @brief Einfaches Beispiel für ESP32_AsyncWebController
 * 
 * Dieses Beispiel zeigt die grundlegende Verwendung der Bibliothek
 * zur Steuerung von 4 GPIOs über einen Webserver.
 */

#include <Arduino.h>
#include <ESP32_AsyncWebController.h>

// ============================================================
// Konfiguration
// ============================================================

// WiFi Access Point
const char* AP_SSID = "ESP32-Demo";
const char* AP_PASSWORD = "";  // Leer = Offenes Netzwerk

// GPIO Pins für 4 Ausgänge (z.B. LEDs oder Relais)
const uint8_t OUTPUT_PINS[] = {2, 4, 5, 12};
const uint8_t NUM_OUTPUTS = 4;

// ============================================================
// Globale Variablen
// ============================================================

// Webserver erstellen (Port 80, 4 Kanäle)
ESP32_AsyncWebController webServer(80, NUM_OUTPUTS);

// Status-Array für Ausgänge
bool outputStates[NUM_OUTPUTS] = {false};

// ============================================================
// Callback-Funktionen
// ============================================================

/**
 * @brief Setze Ausgang (vom Webserver aufgerufen)
 */
void setOutput(uint8_t channel, bool state) {
  if (channel < NUM_OUTPUTS) {
    outputStates[channel] = state;
    digitalWrite(OUTPUT_PINS[channel], state ? HIGH : LOW);
    Serial.printf("GPIO %d (Channel %d) -> %s\n", 
                  OUTPUT_PINS[channel], channel, state ? "ON" : "OFF");
  }
}

/**
 * @brief Hole Ausgang-Status
 */
bool getOutput(uint8_t channel) {
  return channel < NUM_OUTPUTS ? outputStates[channel] : false;
}

/**
 * @brief Alle Zustände als JSON
 */
String getAllStates() {
  String json = "{\"channels\":{";
  for (int i = 0; i < NUM_OUTPUTS; i++) {
    json += "\"" + String(i) + "\":" + (outputStates[i] ? "true" : "false");
    if (i < NUM_OUTPUTS - 1) json += ",";
  }
  json += "}}";
  return json;
}

// ============================================================
// Setup & Loop
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 AsyncWebController - Basic Example ===\n");
  
  // GPIOs als Ausgänge initialisieren
  for (int i = 0; i < NUM_OUTPUTS; i++) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
    digitalWrite(OUTPUT_PINS[i], LOW);
  }
  
  // WiFi Access Point starten
  webServer.startAP(AP_SSID, AP_PASSWORD);
  
  // System-Name setzen
  webServer.setSystemName("ESP32 Basic Demo");
  
  // CORS aktivieren (für externe Apps)
  webServer.enableCORS(true);
  
  // Callbacks registrieren
  webServer.setCallbacks(setOutput, getOutput, getAllStates);
  
  // Server starten
  webServer.begin();
  
  Serial.println("\n=== Setup Complete ===");
  Serial.printf("Open: http://%s\n\n", webServer.getIP().c_str());
}

void loop() {
  // WebSocket Cleanup
  webServer.loop();
  delay(10);
}
