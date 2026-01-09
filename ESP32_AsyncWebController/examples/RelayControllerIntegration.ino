/**
 * @file RelayControllerIntegration.ino
 * @brief Integration von ESP32_AsyncWebController mit LatchController
 * 
 * Zeigt die vollständige Integration für ein 8-Kanal Relais-Board
 * mit 74HC595 Shift Register.
 */

#include <Arduino.h>
#include <LatchController.h>
#include <drivers/ShiftRegisterDriver.h>
#include <ESP32_AsyncWebController.h>

// ============================================================
// Hardware-Konfiguration
// ============================================================

// 74HC595 Pins
#define PIN_DATA  23
#define PIN_CLOCK 18
#define PIN_LATCH 19

// WiFi
const char* AP_SSID = "ESP32-Relays";
const char* AP_PASSWORD = "12345678";

// ============================================================
// Hardware-Komponenten
// ============================================================

ShiftRegisterDriver relayDriver(PIN_DATA, PIN_CLOCK, PIN_LATCH);
LatchController relays(&relayDriver, 8);
ESP32_AsyncWebController webServer(80, 8);

// Thread-Safety für Multi-Core
SemaphoreHandle_t relayMutex;

// ============================================================
// Callbacks
// ============================================================

void setRelay(uint8_t channel, bool state) {
  if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    state ? relays.setLatchOn(channel) : relays.setLatchOff(channel);
    xSemaphoreGive(relayMutex);
  }
}

bool getRelay(uint8_t channel) {
  bool state = false;
  if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    state = relays.getLatchState(channel);
    xSemaphoreGive(relayMutex);
  }
  return state;
}

String getAllRelays() {
  String json = "{\"channels\":{";
  if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    for (int i = 0; i < 8; i++) {
      json += "\"" + String(i) + "\":" + (relays.getLatchState(i) ? "true" : "false");
      if (i < 7) json += ",";
    }
    xSemaphoreGive(relayMutex);
  }
  json += "}}";
  return json;
}

// ============================================================
// Tasks
// ============================================================

void webServerTask(void* param) {
  webServer.startAP(AP_SSID, AP_PASSWORD);
  webServer.setSystemName("8-Channel Relay Controller");
  webServer.setCallbacks(setRelay, getRelay, getAllRelays);
  
  // Custom Route: Alle Relais ausschalten
  webServer.addRoute("/api/alloff", HTTP_POST, [](AsyncWebServerRequest* req) {
    if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      relays.setAllOff();
      xSemaphoreGive(relayMutex);
      req->send(200, "application/json", "{\"success\":true}");
    } else {
      req->send(500, "application/json", "{\"success\":false}");
    }
  });
  
  webServer.begin();
  
  for (;;) {
    webServer.loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void relayTask(void* param) {
  if (xSemaphoreTake(relayMutex, portMAX_DELAY) == pdTRUE) {
    relays.begin(ACTIVE_LOW);
    relays.setAllOff();
    xSemaphoreGive(relayMutex);
  }
  
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ============================================================
// Setup
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  relayMutex = xSemaphoreCreateMutex();
  
  xTaskCreatePinnedToCore(relayTask, "Relay", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(webServerTask, "Web", 8192, NULL, 2, NULL, 0);
  
  Serial.println("System started!");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
