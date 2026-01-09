/**
 * @file ESP32_RelayController.h
 * @brief ESP32 Relais-Controller mit 74HC595 Schieberegister
 * @version 1.0.0
 * @date 2026-01-09
 * 
 * Diese Bibliothek ermöglicht die einfache Steuerung von bis zu 8 Relais
 * über einen 74HC595 Schieberegister mit einem ESP32.
 * 
 * Hardware-Anforderungen:
 * - ESP32 Development Board
 * - 74HC595 Schieberegister
 * - 8-Kanal Relaismodul (empfohlen: LOW-Trigger oder HIGH-Trigger)
 * - Externes 5V Netzteil für Relais (min. 1A)
 * 
 * Wichtig:
 * - Gemeinsame GND-Verbindung zwischen ESP32, 74HC595 und Relaismodul
 * - 74HC595 darf NICHT direkt an Relais angeschlossen werden ohne Treiber!
 * - Verwende ein fertiges Relaismodul oder Transistor + Freilaufdiode
 */

#ifndef ESP32_RELAY_CONTROLLER_H
#define ESP32_RELAY_CONTROLLER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Standardkonfiguration für Pins (ESP32 DevKit sichere GPIOs)
#define DEFAULT_DATA_PIN    23  // DS (Serial Data Input)
#define DEFAULT_CLOCK_PIN   18  // SHCP (Shift Register Clock)
#define DEFAULT_LATCH_PIN   19  // STCP (Storage Register Clock)
// OE ist optional; wenn nicht genutzt, setze auf 0xFF
#define DEFAULT_OE_PIN      0xFF // OE (Output Enable, active LOW) - optional

// Anzahl der Relais-Kanäle
#define MAX_RELAY_CHANNELS  8

/**
 * @enum RelayTriggerMode
 * @brief Definiert den Trigger-Modus des Relaismoduls
 */
enum RelayTriggerMode {
    HIGH_TRIGGER,  // Relais aktiv bei HIGH (Standard für Transistor-Treiber)
    LOW_TRIGGER    // Relais aktiv bei LOW (typisch für fertige Relaismodule)
};

/**
 * @class ESP32_RelayController
 * @brief Hauptklasse zur Steuerung von Relais über 74HC595
 */
class ESP32_RelayController {
private:
    uint8_t dataPin;          // DS Pin
    uint8_t clockPin;         // SHCP Pin
    uint8_t latchPin;         // STCP Pin
    uint8_t oePin;            // OE Pin
    uint8_t currentState;     // Aktueller Zustand aller 8 Relais (Bitmaske)
    RelayTriggerMode triggerMode;  // Trigger-Modus
    bool initialized;         // Initialisierungsstatus
    // FreeRTOS mutex zum Schutz bei Multi-Task Zugriff
    SemaphoreHandle_t lock;

    /**
     * @brief Sendet die Daten an das Schieberegister
     * @param data 8-Bit Wert für die Ausgänge Q0-Q7
     */
    void shiftOut(uint8_t data);

    /**
     * @brief Aktualisiert die Hardware mit dem aktuellen Zustand
     */
    void updateHardware();

public:
    /**
     * @brief Konstruktor mit Standard-Pins
     */
    ESP32_RelayController();

    /**
     * @brief Konstruktor mit benutzerdefinierten Pins
     * @param data DS Pin (Serial Data)
     * @param clock SHCP Pin (Shift Clock)
     * @param latch STCP Pin (Storage Clock)
     * @param oe OE Pin (Output Enable)
     */
    ESP32_RelayController(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe);

    /**
     * @brief Initialisiert die Hardware (Pins & Modus)
     * @param data DS Pin (Serial Data)
     * @param clock SHCP Pin (Shift Clock)
     * @param latch STCP Pin (Latch)
     * @param oe OE Pin (Output Enable) oder 0xFF, wenn nicht verwendet
     * @param mode Trigger-Modus (HIGH_TRIGGER oder LOW_TRIGGER)
     * @return true bei Erfolg, false bei Fehler
     */
    bool begin(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe = 0xFF, RelayTriggerMode mode = HIGH_TRIGGER);

    // (private) locking helpers - öffentlich nicht benötigt
    void takeLock();
    void giveLock();

    /**
     * @brief Setzt den Trigger-Modus
     * @param mode HIGH_TRIGGER oder LOW_TRIGGER
     */
    void setTriggerMode(RelayTriggerMode mode);

    /**
     * @brief Schaltet ein einzelnes Relais ein
     * @param channel Relais-Kanal (0-7)
     * @return true bei Erfolg, false bei ungültigem Kanal
     */
    bool setRelayOn(uint8_t channel);

    /**
     * @brief Schaltet ein einzelnes Relais aus
     * @param channel Relais-Kanal (0-7)
     * @return true bei Erfolg, false bei ungültigem Kanal
     */
    bool setRelayOff(uint8_t channel);

    /**
     * @brief Toggelt ein einzelnes Relais (an↔aus)
     * @param channel Relais-Kanal (0-7)
     * @return true bei Erfolg, false bei ungültigem Kanal
     */
    bool toggleRelay(uint8_t channel);

    /**
     * @brief Setzt den Zustand eines Relais
     * @param channel Relais-Kanal (0-7)
     * @param state true = an, false = aus
     * @return true bei Erfolg, false bei ungültigem Kanal
     */
    bool setRelay(uint8_t channel, bool state);

    /**
     * @brief Schaltet alle Relais ein
     */
    void setAllOn();

    /**
     * @brief Schaltet alle Relais aus
     */
    void setAllOff();

    /**
     * @brief Setzt alle Relais nach Bitmaske
     * @param mask 8-Bit Wert (Bit 0 = Relais 0, Bit 7 = Relais 7)
     */
    void setAllByMask(uint8_t mask);

    /**
     * @brief Liest den Zustand eines Relais
     * @param channel Relais-Kanal (0-7)
     * @return true = an, false = aus (oder ungültiger Kanal)
     */
    bool getRelayState(uint8_t channel);

    /**
     * @brief Liest den Zustand aller Relais als Bitmaske
     * @return 8-Bit Wert (Bit 0 = Relais 0, Bit 7 = Relais 7)
     */
    uint8_t getAllStates();

    /**
     * @brief Aktiviert die Ausgänge (OE = LOW)
     */
    void enable();

    /**
     * @brief Deaktiviert die Ausgänge (OE = HIGH, Tri-State)
     */
    void disable();

    /**
     * @brief Prüft ob Controller initialisiert ist
     * @return true wenn initialisiert
     */
    bool isInitialized();

    /**
     * @brief Gibt Debug-Informationen über serielle Schnittstelle aus
     */
    void printDebugInfo();
};

#endif // ESP32_RELAY_CONTROLLER_H
