/**
 * @file ESP32_RelayController.cpp
 * @brief Implementierung des ESP32 Relais-Controllers
 */

#include "ESP32_RelayController.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ==================== Konstruktoren ====================

ESP32_RelayController::ESP32_RelayController() {
    dataPin = DEFAULT_DATA_PIN;
    clockPin = DEFAULT_CLOCK_PIN;
    latchPin = DEFAULT_LATCH_PIN;
    oePin = DEFAULT_OE_PIN;
    currentState = 0x00;
    triggerMode = HIGH_TRIGGER;
    initialized = false;
    lock = NULL;
}

ESP32_RelayController::ESP32_RelayController(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe) {
    dataPin = data;
    clockPin = clock;
    latchPin = latch;
    oePin = oe;
    currentState = 0x00;
    triggerMode = HIGH_TRIGGER;
    initialized = false;
    lock = NULL;
}

// ==================== Initialisierung ====================

bool ESP32_RelayController::begin(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe, RelayTriggerMode mode) {
    // Pins setzen
    dataPin = data;
    clockPin = clock;
    latchPin = latch;
    oePin = oe;

    // Pin-Modus konfigurieren
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);

    if (oePin != 0xFF) {
        pinMode(oePin, OUTPUT);
        digitalWrite(oePin, LOW); // OE aktivieren (wenn verwendet)
    }

    // Trigger-Modus setzen
    triggerMode = mode;

    // Initialer Zustand: Alle Relais aus
    digitalWrite(dataPin, LOW);
    digitalWrite(clockPin, LOW);
    digitalWrite(latchPin, LOW);

    // Mutex erstellen, falls noch nicht
    if (lock == NULL) {
        lock = xSemaphoreCreateMutex();
    }

    // Alle Relais ausschalten
    currentState = 0x00;
    updateHardware();

    initialized = true;

    Serial.println("✓ ESP32_RelayController initialisiert");
    Serial.printf("  - Trigger-Modus: %s\n", 
                  triggerMode == HIGH_TRIGGER ? "HIGH_TRIGGER" : "LOW_TRIGGER");
    if (oePin == 0xFF) {
        Serial.printf("  - Pins: DATA=%d, CLOCK=%d, LATCH=%d, OE=(none)\n", dataPin, clockPin, latchPin);
    } else {
        Serial.printf("  - Pins: DATA=%d, CLOCK=%d, LATCH=%d, OE=%d\n", dataPin, clockPin, latchPin, oePin);
    }

    return true;
}

void ESP32_RelayController::takeLock() {
    if (lock) xSemaphoreTake(lock, portMAX_DELAY);
}

void ESP32_RelayController::giveLock() {
    if (lock) xSemaphoreGive(lock);
}

void ESP32_RelayController::setTriggerMode(RelayTriggerMode mode) {
    if (triggerMode != mode) {
        triggerMode = mode;
        // Zustand sofort aktualisieren
        updateHardware();
        Serial.printf("→ Trigger-Modus geändert: %s\n", 
                      mode == HIGH_TRIGGER ? "HIGH_TRIGGER" : "LOW_TRIGGER");
    }
}

// ==================== Private Hilfsfunktionen ====================

void ESP32_RelayController::shiftOut(uint8_t data) {
    // MSB-First Übertragung (Bit 7 zuerst)
    for (int8_t i = 7; i >= 0; i--) {
        // Clock LOW
        digitalWrite(clockPin, LOW);
        
        // Datenbit setzen
        digitalWrite(dataPin, (data & (1 << i)) ? HIGH : LOW);
        
        // Clock HIGH (Datenübernahme)
        digitalWrite(clockPin, HIGH);
    }
    
    // Clock wieder LOW
    digitalWrite(clockPin, LOW);
}

void ESP32_RelayController::updateHardware() {
    if (!initialized) return;

    takeLock();

    // Bei LOW_TRIGGER: Logik invertieren
    uint8_t outputData = (triggerMode == LOW_TRIGGER) ? ~currentState : currentState;

    // Latch LOW (Vorbereitung)
    digitalWrite(latchPin, LOW);
    
    // Daten senden
    shiftOut(outputData);
    
    // Latch HIGH (Ausgänge aktualisieren)
    digitalWrite(latchPin, HIGH);

    giveLock();
}

// ==================== Einzelne Relais steuern ====================

bool ESP32_RelayController::setRelayOn(uint8_t channel) {
    if (channel >= MAX_RELAY_CHANNELS) {
        Serial.printf("✗ Fehler: Kanal %d ungültig (0-%d)\n", channel, MAX_RELAY_CHANNELS - 1);
        return false;
    }

    currentState |= (1 << channel);  // Bit setzen
    updateHardware();
    
    return true;
}

bool ESP32_RelayController::setRelayOff(uint8_t channel) {
    if (channel >= MAX_RELAY_CHANNELS) {
        Serial.printf("✗ Fehler: Kanal %d ungültig (0-%d)\n", channel, MAX_RELAY_CHANNELS - 1);
        return false;
    }

    currentState &= ~(1 << channel);  // Bit löschen
    updateHardware();
    
    return true;
}

bool ESP32_RelayController::toggleRelay(uint8_t channel) {
    if (channel >= MAX_RELAY_CHANNELS) {
        Serial.printf("✗ Fehler: Kanal %d ungültig (0-%d)\n", channel, MAX_RELAY_CHANNELS - 1);
        return false;
    }

    currentState ^= (1 << channel);  // Bit toggeln
    updateHardware();
    
    return true;
}

bool ESP32_RelayController::setRelay(uint8_t channel, bool state) {
    return state ? setRelayOn(channel) : setRelayOff(channel);
}

// ==================== Alle Relais steuern ====================

void ESP32_RelayController::setAllOn() {
    currentState = 0xFF;  // Alle 8 Bits auf 1
    updateHardware();
    Serial.println("→ Alle Relais EIN");
}

void ESP32_RelayController::setAllOff() {
    currentState = 0x00;  // Alle 8 Bits auf 0
    updateHardware();
    Serial.println("→ Alle Relais AUS");
}

void ESP32_RelayController::setAllByMask(uint8_t mask) {
    currentState = mask;
    updateHardware();
    Serial.printf("→ Relais-Maske: 0x%02X (binär: ", mask);
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((mask & (1 << i)) ? '1' : '0');
    }
    Serial.println(")");
}

// ==================== Status abfragen ====================

bool ESP32_RelayController::getRelayState(uint8_t channel) {
    if (channel >= MAX_RELAY_CHANNELS) {
        return false;
    }
    return (currentState & (1 << channel)) != 0;
}

uint8_t ESP32_RelayController::getAllStates() {
    return currentState;
}

// ==================== Output Enable ====================

void ESP32_RelayController::enable() {
    digitalWrite(oePin, LOW);  // OE ist active LOW
    Serial.println("→ Ausgänge aktiviert (OE = LOW)");
}

void ESP32_RelayController::disable() {
    digitalWrite(oePin, HIGH);  // OE HIGH = Tri-State
    Serial.println("→ Ausgänge deaktiviert (OE = HIGH, Tri-State)");
}

// ==================== Utilities ====================

bool ESP32_RelayController::isInitialized() {
    return initialized;
}

void ESP32_RelayController::printDebugInfo() {
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║   ESP32 Relais-Controller Status          ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.printf("║ Initialisiert:     %-20s ║\n", initialized ? "✓ JA" : "✗ NEIN");
    Serial.printf("║ Trigger-Modus:     %-20s ║\n", 
                  triggerMode == HIGH_TRIGGER ? "HIGH_TRIGGER" : "LOW_TRIGGER");
    Serial.printf("║ DATA Pin:          GPIO %-16d ║\n", dataPin);
    Serial.printf("║ CLOCK Pin:         GPIO %-16d ║\n", clockPin);
    Serial.printf("║ LATCH Pin:         GPIO %-16d ║\n", latchPin);
    Serial.printf("║ OE Pin:            GPIO %-16d ║\n", oePin);
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.printf("║ Aktueller Zustand: 0x%02X                   ║\n", currentState);
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.println("║ Relais-Kanäle (0-7):                       ║");
    
    for (uint8_t i = 0; i < MAX_RELAY_CHANNELS; i++) {
        bool state = getRelayState(i);
        Serial.printf("║   Kanal %d:          %s                   ║\n", 
                      i, state ? "🟢 EIN " : "⚫ AUS");
    }
    
    Serial.println("╚════════════════════════════════════════════╝\n");
}
