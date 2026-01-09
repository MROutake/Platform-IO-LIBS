/**
 * @file LatchController.cpp
 * @brief Implementierung der abstrakten Latch-Controller Library
 */

#include "LatchController.h"

// ==================== LatchController ====================

LatchController::LatchController(LatchDriver* drv, uint8_t channels) {
    driver = drv;
    channelCount = channels;
    currentState = 0;
    triggerMode = ACTIVE_HIGH;
    initialized = false;
    lock = NULL;
}

LatchController::~LatchController() {
    if (lock) {
        vSemaphoreDelete(lock);
    }
}

bool LatchController::begin(LatchTriggerMode mode) {
    if (!driver) {
        Serial.println("✗ Fehler: Kein Driver gesetzt!");
        return false;
    }

    // Mutex erstellen
    if (lock == NULL) {
        lock = xSemaphoreCreateMutex();
    }

    // Trigger-Modus setzen
    triggerMode = mode;

    // Driver initialisieren
    if (!driver->init()) {
        Serial.println("✗ Fehler: Driver-Initialisierung fehlgeschlagen!");
        return false;
    }

    // Alle Latches ausschalten
    currentState = 0;
    driver->updateHardware(currentState, channelCount);

    initialized = true;

    Serial.println("✓ LatchController initialisiert");
    Serial.printf("  - Driver: %s\n", driver->getName());
    Serial.printf("  - Kanäle: %d\n", channelCount);
    Serial.printf("  - Trigger-Modus: %s\n", 
                  triggerMode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");

    return true;
}

void LatchController::takeLock() {
    if (lock) xSemaphoreTake(lock, portMAX_DELAY);
}

void LatchController::giveLock() {
    if (lock) xSemaphoreGive(lock);
}

bool LatchController::setLatch(uint8_t channel, bool state) {
    if (channel >= channelCount) {
        Serial.printf("✗ Fehler: Kanal %d ungültig (0-%d)\n", channel, channelCount - 1);
        return false;
    }

    takeLock();

    if (state) {
        currentState |= (1UL << channel);  // Bit setzen
    } else {
        currentState &= ~(1UL << channel); // Bit löschen
    }

    // Bei ACTIVE_LOW: Logik invertieren
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);

    giveLock();
    return true;
}

bool LatchController::setLatchOn(uint8_t channel) {
    return setLatch(channel, true);
}

bool LatchController::setLatchOff(uint8_t channel) {
    return setLatch(channel, false);
}

bool LatchController::toggleLatch(uint8_t channel) {
    if (channel >= channelCount) {
        Serial.printf("✗ Fehler: Kanal %d ungültig (0-%d)\n", channel, channelCount - 1);
        return false;
    }

    takeLock();
    currentState ^= (1UL << channel);  // Bit toggeln
    
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);
    
    giveLock();
    return true;
}

void LatchController::setAllLatches(uint32_t mask) {
    takeLock();
    
    // Maske auf channelCount begrenzen
    uint32_t channelMask = (1UL << channelCount) - 1;
    currentState = mask & channelMask;
    
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);
    
    giveLock();
}

void LatchController::setAllOn() {
    uint32_t mask = (1UL << channelCount) - 1;  // Alle Bits setzen
    setAllLatches(mask);
    Serial.println("→ Alle Latches EIN");
}

void LatchController::setAllOff() {
    setAllLatches(0);
    Serial.println("→ Alle Latches AUS");
}

bool LatchController::getLatchState(uint8_t channel) {
    if (channel >= channelCount) {
        return false;
    }
    return (currentState & (1UL << channel)) != 0;
}

uint32_t LatchController::getAllStates() {
    return currentState;
}

uint8_t LatchController::getChannelCount() {
    return channelCount;
}

void LatchController::setTriggerMode(LatchTriggerMode mode) {
    if (triggerMode != mode) {
        takeLock();
        triggerMode = mode;
        
        // Zustand sofort aktualisieren
        uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
        driver->updateHardware(outputData, channelCount);
        
        giveLock();
        
        Serial.printf("→ Trigger-Modus geändert: %s\n", 
                      mode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");
    }
}

bool LatchController::isInitialized() {
    return initialized;
}

void LatchController::printDebugInfo() {
    Serial.println("\n╔════════════════════════════════════════════╗");
    Serial.println("║   Latch Controller Status                  ║");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.printf("║ Driver:            %-20s ║\n", driver ? driver->getName() : "KEIN");
    Serial.printf("║ Initialisiert:     %-20s ║\n", initialized ? "✓ JA" : "✗ NEIN");
    Serial.printf("║ Kanäle:            %-20d ║\n", channelCount);
    Serial.printf("║ Trigger-Modus:     %-20s ║\n", 
                  triggerMode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.printf("║ Aktueller Zustand: 0x%08X             ║\n", currentState);
    Serial.println("╠════════════════════════════════════════════╣");
    Serial.println("║ Latch-Kanäle:                              ║");
    
    for (uint8_t i = 0; i < channelCount; i++) {
        bool state = getLatchState(i);
        Serial.printf("║   Kanal %2d:        %s                   ║\n", 
                      i, state ? "🟢 EIN " : "⚫ AUS");
    }
    
    Serial.println("╚════════════════════════════════════════════╝\n");
}
