/**
 * @file DirectLatchDriver.h
 * @brief Driver für direkte D-Latch ICs (74HC373, 74HC75, etc.)
 * 
 * Unterstützt:
 * - 74HC373 (8x D-Latch mit gemeinsamem Enable)
 * - 74HC75 (4x D-Latch)
 * - CD4042 (4x D-Latch)
 * 
 * Funktionsweise:
 * - Jeder Kanal hat einen eigenen DATA-Pin
 * - Ein gemeinsames ENABLE-Signal
 * - EN=HIGH → Ausgänge folgen Eingängen
 * - EN=LOW → Ausgänge halten Wert
 */

#ifndef DIRECT_LATCH_DRIVER_H
#define DIRECT_LATCH_DRIVER_H

#include "../LatchController.h"

/**
 * @class DirectLatchDriver
 * @brief Driver für parallele D-Latch ICs
 */
class DirectLatchDriver : public LatchDriver {
private:
    uint8_t* dataPins;      // Array mit DATA-Pins für jeden Kanal
    uint8_t enablePin;      // Gemeinsamer Enable-Pin
    uint8_t numChannels;    // Anzahl Kanäle

public:
    /**
     * @brief Konstruktor
     * @param pins Array mit GPIO-Pins für D0-D7
     * @param enable Enable-Pin
     * @param channels Anzahl Kanäle
     */
    DirectLatchDriver(uint8_t* pins, uint8_t enable, uint8_t channels);
    ~DirectLatchDriver();

    bool init() override;
    void updateHardware(uint32_t data, uint8_t channelCount) override;
    const char* getName() override;
    uint8_t getMaxChannels() override;
};

#endif // DIRECT_LATCH_DRIVER_H
