/**
 * @file LatchController.h
 * @brief Professional Latch Controller Library for ESP32
 * @version 3.0.0
 * @author MROutake
 * @date 2025
 * 
 * Industrial-grade library for controlling various latch ICs.
 * Supports multiple IC types through modular driver architecture.
 * 
 * Supported Latch Types:
 * - Shift Registers (74HC595, 74HC164, etc.)
 * - Direct D-Latches (74HC373, 74HC75, etc.)
 * - I2C Expanders (PCF8574, MCP23017, etc.)
 * - SPI Expanders (MCP23S17, etc.)
 * 
 * Features:
 * - Thread-safe with FreeRTOS mutex
 * - Modular driver architecture
 * - Supports up to 32 channels
 * - ACTIVE_HIGH / ACTIVE_LOW hardware support
 * 
 * FreeRTOS Best Practice:
 * - Run GPIO/Hardware tasks on Core 1
 * - Run Network/WiFi tasks on Core 0
 */

#ifndef LATCH_CONTROLLER_H
#define LATCH_CONTROLLER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ============================================================
// Version
// ============================================================
#define LATCH_CONTROLLER_VERSION "3.0.0"

// Forward declaration
class LatchDriver;

// ============================================================
// Enums
// ============================================================

/**
 * @enum LatchTriggerMode
 * @brief Defines the hardware trigger logic
 */
enum LatchTriggerMode {
    ACTIVE_HIGH,  ///< Standard logic: HIGH = device active
    ACTIVE_LOW    ///< Inverted logic: LOW = device active (typical for relay modules)
};

// ============================================================
// LatchController Class
// ============================================================

/**
 * @class LatchController
 * @brief Main class for abstract latch control
 * 
 * Thread-safe controller for various latch ICs.
 * The specific IC type is defined through a modular driver.
 * 
 * @note setLatchOn/Off always represent LOGICAL state.
 *       Hardware inversion for ACTIVE_LOW is handled internally.
 */
class LatchController {
private:
    LatchDriver* driver;
    uint8_t channelCount;
    uint32_t currentState;
    LatchTriggerMode triggerMode;
    bool initialized;
    SemaphoreHandle_t lock;

    void takeLock();
    void giveLock();

public:
    /**
     * @brief Constructor
     * @param driver Hardware driver for the IC type
     * @param channels Number of channels (e.g., 8 for 74HC595)
     */
    LatchController(LatchDriver* driver, uint8_t channels = 8);

    /**
     * @brief Destructor - cleans up mutex
     */
    ~LatchController();

    /**
     * @brief Initialize the controller
     * @param mode Trigger mode (ACTIVE_HIGH or ACTIVE_LOW)
     * @return true on success
     */
    bool begin(LatchTriggerMode mode = ACTIVE_HIGH);

    // ========== Single Channel Control ==========

    /**
     * @brief Set a single latch state
     * @param channel Channel number (0 to channelCount-1)
     * @param state true = ON (logical), false = OFF (logical)
     * @return true on success
     * @note Hardware inversion for ACTIVE_LOW is handled internally
     */
    bool setLatch(uint8_t channel, bool state);

    /**
     * @brief Turn a latch ON (logical state)
     * @param channel Channel number
     * @return true on success
     */
    bool setLatchOn(uint8_t channel);

    /**
     * @brief Turn a latch OFF (logical state)
     * @param channel Channel number
     * @return true on success
     */
    bool setLatchOff(uint8_t channel);

    /**
     * @brief Toggle a latch state
     * @param channel Channel number
     * @return true on success
     */
    bool toggleLatch(uint8_t channel);

    // ========== Bulk Control ==========

    /**
     * @brief Set all latches from bit mask
     * @param mask Bit mask (bit 0 = channel 0, etc.)
     */
    void setAllLatches(uint32_t mask);

    /**
     * @brief Turn all latches ON
     */
    void setAllOn();

    /**
     * @brief Turn all latches OFF
     */
    void setAllOff();

    // ========== State Query ==========

    /**
     * @brief Get single latch state (logical)
     * @param channel Channel number
     * @return true if ON (logical), false if OFF
     */
    bool getLatchState(uint8_t channel);

    /**
     * @brief Get all latch states as bit mask (logical)
     * @return Bit mask of all states
     */
    uint32_t getAllStates();

    // ========== Configuration ==========

    /**
     * @brief Get channel count
     * @return Number of configured channels
     */
    uint8_t getChannelCount();

    /**
     * @brief Change trigger mode at runtime
     * @param mode New trigger mode
     */
    void setTriggerMode(LatchTriggerMode mode);

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized();

    /**
     * @brief Print debug information to Serial
     */
    void printDebugInfo();
};

// ============================================================
// LatchDriver Interface (Abstract Base Class)
// ============================================================

/**
 * @class LatchDriver
 * @brief Abstract base class for hardware drivers
 * 
 * Implement this interface to support new IC types.
 */
class LatchDriver {
public:
    virtual ~LatchDriver() {}

    /**
     * @brief Initialize the hardware
     * @return true on success
     */
    virtual bool init() = 0;

    /**
     * @brief Update hardware output
     * @param data Bit pattern to output (already inverted if ACTIVE_LOW)
     * @param channelCount Number of channels to update
     */
    virtual void updateHardware(uint32_t data, uint8_t channelCount) = 0;

    /**
     * @brief Get driver name for debugging
     * @return Driver name string
     */
    virtual const char* getName() = 0;

    /**
     * @brief Get maximum supported channels
     * @return Maximum channel count
     */
    virtual uint8_t getMaxChannels() = 0;
};

#endif // LATCH_CONTROLLER_H
