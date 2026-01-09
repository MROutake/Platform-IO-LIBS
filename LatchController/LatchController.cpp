/**
 * @file LatchController.cpp
 * @brief Professional Latch Controller Implementation
 * @version 3.0.0
 */

#include "LatchController.h"

// ============================================================
// LatchController Implementation
// ============================================================

LatchController::LatchController(LatchDriver* drv, uint8_t channels) 
    : driver(drv)
    , channelCount(min(channels, (uint8_t)32))  // Max 32 channels
    , currentState(0)
    , triggerMode(ACTIVE_HIGH)
    , initialized(false)
    , lock(nullptr)
{
}

LatchController::~LatchController() {
    if (lock != nullptr) {
        vSemaphoreDelete(lock);
        lock = nullptr;
    }
}

bool LatchController::begin(LatchTriggerMode mode) {
    // Validate driver
    if (driver == nullptr) {
        Serial.println("[LatchController] ERROR: No driver attached!");
        return false;
    }

    // Create mutex if not exists
    if (lock == nullptr) {
        lock = xSemaphoreCreateMutex();
        if (lock == nullptr) {
            Serial.println("[LatchController] ERROR: Failed to create mutex!");
            return false;
        }
    }

    triggerMode = mode;

    // Initialize driver
    if (!driver->init()) {
        Serial.println("[LatchController] ERROR: Driver init failed!");
        return false;
    }

    // Set all outputs OFF (considering trigger mode)
    currentState = 0;
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);

    initialized = true;

    Serial.println("[LatchController] Initialized successfully");
    Serial.printf("  Driver:   %s\n", driver->getName());
    Serial.printf("  Channels: %d\n", channelCount);
    Serial.printf("  Mode:     %s\n", triggerMode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");

    return true;
}

void LatchController::takeLock() {
    if (lock != nullptr) {
        xSemaphoreTake(lock, portMAX_DELAY);
    }
}

void LatchController::giveLock() {
    if (lock != nullptr) {
        xSemaphoreGive(lock);
    }
}

bool LatchController::setLatch(uint8_t channel, bool state) {
    if (channel >= channelCount) {
        Serial.printf("[LatchController] ERROR: Invalid channel %d (max: %d)\n", 
                      channel, channelCount - 1);
        return false;
    }

    takeLock();

    // Update internal logical state
    if (state) {
        currentState |= (1UL << channel);
    } else {
        currentState &= ~(1UL << channel);
    }

    // Apply hardware inversion for ACTIVE_LOW
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);

    giveLock();
    return true;
}

bool LatchController::setLatchOn(uint8_t channel) {
    // Logical ON = state true
    // Hardware inversion is handled in setLatch()
    return setLatch(channel, true);
}

bool LatchController::setLatchOff(uint8_t channel) {
    // Logical OFF = state false
    // Hardware inversion is handled in setLatch()
    return setLatch(channel, false);
}

bool LatchController::toggleLatch(uint8_t channel) {
    if (channel >= channelCount) {
        Serial.printf("[LatchController] ERROR: Invalid channel %d\n", channel);
        return false;
    }

    takeLock();
    
    currentState ^= (1UL << channel);
    
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);
    
    giveLock();
    return true;
}

void LatchController::setAllLatches(uint32_t mask) {
    takeLock();
    
    // Limit mask to valid channels
    uint32_t channelMask = (1UL << channelCount) - 1;
    currentState = mask & channelMask;
    
    uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
    driver->updateHardware(outputData, channelCount);
    
    giveLock();
}

void LatchController::setAllOn() {
    uint32_t mask = (1UL << channelCount) - 1;
    setAllLatches(mask);
    Serial.println("[LatchController] All latches ON");
}

void LatchController::setAllOff() {
    setAllLatches(0);
    Serial.println("[LatchController] All latches OFF");
}

bool LatchController::getLatchState(uint8_t channel) {
    if (channel >= channelCount) {
        return false;
    }
    // Return logical state (not hardware state)
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
        
        // Update hardware with new mode
        uint32_t outputData = (triggerMode == ACTIVE_LOW) ? ~currentState : currentState;
        driver->updateHardware(outputData, channelCount);
        
        giveLock();
        
        Serial.printf("[LatchController] Trigger mode: %s\n", 
                      mode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");
    }
}

bool LatchController::isInitialized() {
    return initialized;
}

void LatchController::printDebugInfo() {
    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║       LatchController v3.0.0             ║");
    Serial.println("╠══════════════════════════════════════════╣");
    Serial.printf("║ Driver:      %-26s ║\n", driver ? driver->getName() : "NONE");
    Serial.printf("║ Initialized: %-26s ║\n", initialized ? "Yes" : "No");
    Serial.printf("║ Channels:    %-26d ║\n", channelCount);
    Serial.printf("║ Mode:        %-26s ║\n", 
                  triggerMode == ACTIVE_HIGH ? "ACTIVE_HIGH" : "ACTIVE_LOW");
    Serial.printf("║ State:       0x%08X                 ║\n", currentState);
    Serial.println("╠══════════════════════════════════════════╣");
    
    for (uint8_t i = 0; i < channelCount; i++) {
        bool state = getLatchState(i);
        Serial.printf("║ Channel %2d:  %s                       ║\n", 
                      i, state ? "ON " : "OFF");
    }
    
    Serial.println("╚══════════════════════════════════════════╝");
    Serial.println();
}
