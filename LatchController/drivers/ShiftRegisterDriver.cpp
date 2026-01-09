/**
 * @file ShiftRegisterDriver.cpp
 * @brief Implementierung des Shift-Register Drivers
 */

#include "ShiftRegisterDriver.h"

// Konstruktor für 74HC595 (mit Latch)
ShiftRegisterDriver::ShiftRegisterDriver(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe) {
    dataPin = data;
    clockPin = clock;
    latchPin = latch;
    oePin = oe;
    type = SR_74HC595;
}

// Konstruktor für 74HC164 (ohne Latch)
ShiftRegisterDriver::ShiftRegisterDriver(uint8_t data, uint8_t clock) {
    dataPin = data;
    clockPin = clock;
    latchPin = 0xFF;
    oePin = 0xFF;
    type = SR_74HC164;
}

bool ShiftRegisterDriver::init() {
    // Pins konfigurieren
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    
    if (latchPin != 0xFF) {
        pinMode(latchPin, OUTPUT);
        digitalWrite(latchPin, LOW);
    }
    
    if (oePin != 0xFF) {
        pinMode(oePin, OUTPUT);
        digitalWrite(oePin, LOW);  // Output Enable aktiv
    }
    
    digitalWrite(dataPin, LOW);
    digitalWrite(clockPin, LOW);
    
    // Small delay for hardware stabilization
    delayMicroseconds(10);
    
    // Clear shift register with all HIGH (for ACTIVE_LOW relays = all OFF)
    // This ensures defined state at startup before LatchController sets actual values
    for (int i = 0; i < 8; i++) {
        digitalWrite(dataPin, HIGH);  // HIGH = Relay OFF for ACTIVE_LOW
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
    if (latchPin != 0xFF) {
        digitalWrite(latchPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(latchPin, LOW);
    }
    
    Serial.printf("[ShiftRegister] Initialized: %s\n", getName());
    Serial.printf("  DATA=%d, CLOCK=%d", dataPin, clockPin);
    if (latchPin != 0xFF) Serial.printf(", LATCH=%d", latchPin);
    if (oePin != 0xFF) Serial.printf(", OE=%d", oePin);
    Serial.println();
    
    return true;
}

void ShiftRegisterDriver::shiftOut(uint32_t data, uint8_t bits) {
    // MSB-First Übertragung
    for (int8_t i = bits - 1; i >= 0; i--) {
        digitalWrite(clockPin, LOW);
        digitalWrite(dataPin, (data & (1UL << i)) ? HIGH : LOW);
        digitalWrite(clockPin, HIGH);
    }
    digitalWrite(clockPin, LOW);
}

void ShiftRegisterDriver::updateHardware(uint32_t data, uint8_t channelCount) {
    // Latch LOW (wenn vorhanden)
    if (latchPin != 0xFF) {
        digitalWrite(latchPin, LOW);
    }
    
    // Daten schieben
    shiftOut(data, channelCount);
    
    // Latch HIGH → Daten übernehmen
    if (latchPin != 0xFF) {
        digitalWrite(latchPin, HIGH);
    }
}

const char* ShiftRegisterDriver::getName() {
    switch (type) {
        case SR_74HC595:  return "74HC595 Shift Register";
        case SR_74HC164:  return "74HC164 Shift Register";
        case SR_74HC4094: return "74HC4094 Shift Register";
        default:          return "Unknown Shift Register";
    }
}

uint8_t ShiftRegisterDriver::getMaxChannels() {
    return 32;  // Theoretisch unbegrenzt kaskadierbar, praktisch 32 Bit
}
