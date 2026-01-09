/**
 * @file LatchController.h
 * @brief Universal Latch Controller Library for ESP32
 * @version 2.0.0
 * @date 2026-01-09
 * 
 * Abstrakte Library zur Steuerung verschiedener Latch-ICs.
 * Unterstützt verschiedene IC-Typen durch austauschbare Driver.
 * 
 * Unterstützte Latch-Typen:
 * - Shift Register (74HC595, 74HC164, etc.)
 * - Direct D-Latches (74HC373, 74HC75, etc.)
 * - I2C Expander (PCF8574, MCP23017, etc.)
 * - SPI Expander (MCP23S17, etc.)
 * 
 * Thread-safe mit FreeRTOS Mutex.
 */

#ifndef LATCH_CONTROLLER_H
#define LATCH_CONTROLLER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Forward declaration
class LatchDriver;

/**
 * @enum LatchTriggerMode
 * @brief Definiert die Logik des Latches
 */
enum LatchTriggerMode {
    ACTIVE_HIGH,  // Latch aktiv bei HIGH (Standard für D-Latch)
    ACTIVE_LOW    // Latch aktiv bei LOW (typisch für Relaismodule)
};

/**
 * @class LatchController
 * @brief Hauptklasse zur abstrakten Steuerung von Latches
 * 
 * Diese Klasse bietet eine einheitliche API für verschiedene Latch-ICs.
 * Der konkrete IC-Typ wird über einen austauschbaren Driver definiert.
 */
class LatchController {
private:
    LatchDriver* driver;              // Pointer zum Hardware-Driver
    uint8_t channelCount;             // Anzahl der Kanäle
    uint32_t currentState;            // Aktueller Zustand (32 Bit für bis zu 32 Latches)
    LatchTriggerMode triggerMode;     // Trigger-Modus
    bool initialized;                 // Initialisierungsstatus
    SemaphoreHandle_t lock;           // FreeRTOS mutex

    void takeLock();
    void giveLock();

public:
    /**
     * @brief Konstruktor
     * @param driver Hardware-Driver für den IC-Typ
     * @param channels Anzahl der Kanäle (z.B. 8 für 74HC595)
     */
    LatchController(LatchDriver* driver, uint8_t channels = 8);

    /**
     * @brief Destruktor
     */
    ~LatchController();

    /**
     * @brief Initialisiert den Controller
     * @param mode Trigger-Modus (ACTIVE_HIGH oder ACTIVE_LOW)
     * @return true bei Erfolg
     */
    bool begin(LatchTriggerMode mode = ACTIVE_HIGH);

    /**
     * @brief Setzt ein einzelnes Latch
     * @param channel Kanal (0 bis channelCount-1)
     * @param state true = aktiv, false = inaktiv
     * @return true bei Erfolg
     */
    bool setLatch(uint8_t channel, bool state);

    /**
     * @brief Schaltet ein Latch ein
     * @param channel Kanal
     * @return true bei Erfolg
     */
    bool setLatchOn(uint8_t channel);

    /**
     * @brief Schaltet ein Latch aus
     * @param channel Kanal
     * @return true bei Erfolg
     */
    bool setLatchOff(uint8_t channel);

    /**
     * @brief Toggelt ein Latch
     * @param channel Kanal
     * @return true bei Erfolg
     */
    bool toggleLatch(uint8_t channel);

    /**
     * @brief Setzt alle Latches nach Bitmaske
     * @param mask Bitmaske (Bit 0 = Kanal 0, etc.)
     */
    void setAllLatches(uint32_t mask);

    /**
     * @brief Schaltet alle Latches ein
     */
    void setAllOn();

    /**
     * @brief Schaltet alle Latches aus
     */
    void setAllOff();

    /**
     * @brief Liest den Zustand eines Latches
     * @param channel Kanal
     * @return true = aktiv, false = inaktiv
     */
    bool getLatchState(uint8_t channel);

    /**
     * @brief Liest alle Latch-Zustände als Bitmaske
     * @return Bitmaske aller Kanäle
     */
    uint32_t getAllStates();

    /**
     * @brief Anzahl der verfügbaren Kanäle
     * @return Anzahl Kanäle
     */
    uint8_t getChannelCount();

    /**
     * @brief Setzt den Trigger-Modus
     * @param mode ACTIVE_HIGH oder ACTIVE_LOW
     */
    void setTriggerMode(LatchTriggerMode mode);

    /**
     * @brief Prüft ob initialisiert
     * @return true wenn initialisiert
     */
    bool isInitialized();

    /**
     * @brief Debug-Ausgabe
     */
    void printDebugInfo();
};

/**
 * @class LatchDriver
 * @brief Abstrakte Basisklasse für Hardware-Driver
 * 
 * Jeder konkrete IC-Typ (74HC595, 74HC373, etc.) implementiert
 * diese Schnittstelle.
 */
class LatchDriver {
public:
    virtual ~LatchDriver() {}

    /**
     * @brief Initialisiert die Hardware
     * @return true bei Erfolg
     */
    virtual bool init() = 0;

    /**
     * @brief Aktualisiert die Hardware mit neuen Werten
     * @param data Daten für alle Kanäle (Bitmaske)
     * @param channelCount Anzahl der Kanäle
     */
    virtual void updateHardware(uint32_t data, uint8_t channelCount) = 0;

    /**
     * @brief Name des Drivers (für Debug)
     * @return Driver-Name
     */
    virtual const char* getName() = 0;

    /**
     * @brief Maximale Anzahl Kanäle die dieser Driver unterstützt
     * @return Max. Kanäle
     */
    virtual uint8_t getMaxChannels() = 0;
};

#endif // LATCH_CONTROLLER_H
