/**
 * @file ShiftRegisterDriver.h
 * @brief Driver für Shift-Register ICs (74HC595, 74HC164, etc.)
 * 
 * Unterstützt:
 * - 74HC595 (8-Bit mit Storage Register)
 * - 74HC164 (8-Bit ohne Storage Register)
 * - 74HC4094 (8-Bit mit Strobe)
 * - Kaskadierbar für 16, 24, 32+ Bit
 */

#ifndef SHIFT_REGISTER_DRIVER_H
#define SHIFT_REGISTER_DRIVER_H

#include "LatchController.h"

/**
 * @enum ShiftRegisterType
 * @brief Typ des Shift-Registers
 */
enum ShiftRegisterType {
    SR_74HC595,   // Standard: SHCP + STCP
    SR_74HC164,   // Einfach: nur CLOCK
    SR_74HC4094   // Mit STROBE
};

/**
 * @class ShiftRegisterDriver
 * @brief Driver für Shift-Register basierte Latches
 */
class ShiftRegisterDriver : public LatchDriver {
private:
    uint8_t dataPin;      // DS / Serial Data
    uint8_t clockPin;     // SHCP / Clock
    uint8_t latchPin;     // STCP / Latch (nur 74HC595)
    uint8_t oePin;        // Output Enable (optional)
    ShiftRegisterType type;

    void shiftOut(uint32_t data, uint8_t bits);

public:
    /**
     * @brief Konstruktor für 74HC595
     * @param data DS Pin
     * @param clock SHCP Pin
     * @param latch STCP Pin
     * @param oe OE Pin (0xFF wenn nicht verwendet)
     */
    ShiftRegisterDriver(uint8_t data, uint8_t clock, uint8_t latch, uint8_t oe = 0xFF);

    /**
     * @brief Konstruktor für 74HC164 (ohne Latch)
     * @param data DS Pin
     * @param clock Clock Pin
     */
    ShiftRegisterDriver(uint8_t data, uint8_t clock);

    bool init() override;
    void updateHardware(uint32_t data, uint8_t channelCount) override;
    const char* getName() override;
    uint8_t getMaxChannels() override;
};

#endif // SHIFT_REGISTER_DRIVER_H
