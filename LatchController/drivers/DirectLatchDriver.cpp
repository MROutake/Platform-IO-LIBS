/**
 * @file DirectLatchDriver.cpp
 * @brief Implementierung des Direct-Latch Drivers
 */

#include "DirectLatchDriver.h"

DirectLatchDriver::DirectLatchDriver(uint8_t* pins, uint8_t enable, uint8_t channels) {
    numChannels = channels;
    enablePin = enable;
    
    // Pins kopieren
    dataPins = new uint8_t[channels];
    for (uint8_t i = 0; i < channels; i++) {
        dataPins[i] = pins[i];
    }
}

DirectLatchDriver::~DirectLatchDriver() {
    delete[] dataPins;
}

bool DirectLatchDriver::init() {
    // Enable-Pin konfigurieren
    pinMode(enablePin, OUTPUT);
    digitalWrite(enablePin, LOW);  // Erstmal LOW (Latches halten)
    
    // Data-Pins konfigurieren
    for (uint8_t i = 0; i < numChannels; i++) {
        pinMode(dataPins[i], OUTPUT);
        digitalWrite(dataPins[i], LOW);
    }
    
    Serial.printf("✓ DirectLatchDriver initialisiert (74HC373-Style)\n");
    Serial.printf("  - ENABLE=%d\n", enablePin);
    Serial.printf("  - Kanäle: %d (D0-D%d)\n", numChannels, numChannels-1);
    
    return true;
}

void DirectLatchDriver::updateHardware(uint32_t data, uint8_t channelCount) {
    // 1. Enable HIGH → Latches transparent
    digitalWrite(enablePin, HIGH);
    
    // 2. Data-Pins setzen
    for (uint8_t i = 0; i < channelCount && i < numChannels; i++) {
        digitalWrite(dataPins[i], (data & (1UL << i)) ? HIGH : LOW);
    }
    
    // 3. Enable LOW → Werte speichern
    digitalWrite(enablePin, LOW);
}

const char* DirectLatchDriver::getName() {
    return "74HC373 Direct D-Latch";
}

uint8_t DirectLatchDriver::getMaxChannels() {
    return numChannels;
}
