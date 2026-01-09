# LatchController v3.0.0# LatchController v2.0



Professional industrial-grade library for controlling latch ICs on ESP32.🎯 **Universal abstraction library for controlling latches on ESP32**



## FeaturesSupports multiple IC types through exchangeable drivers:

- **Shift Registers:** 74HC595, 74HC164, 74HC4094

- **Modular Driver Architecture** - Support for multiple IC types through interchangeable drivers- **Direct D-Latches:** 74HC373, 74HC75, CD4042

- **Thread-Safe** - FreeRTOS mutex protection for multi-task environments- **Extensible:** Add your own driver for any latch-based IC

- **Up to 32 Channels** - Control large relay banks or LED arrays

- **ACTIVE_HIGH / ACTIVE_LOW** - Automatic hardware inversion for relay modules## Features

- **Clean API** - Simple, intuitive interface- ✅ Abstract API - same code for different ICs

- ✅ Thread-safe (FreeRTOS mutex)

## Supported Hardware- ✅ Support for up to 32 channels

- ✅ ACTIVE_HIGH / ACTIVE_LOW modes

| Driver | IC Types | Protocol |- ✅ Individual & group control

|--------|----------|----------|

| ShiftRegisterDriver | 74HC595, 74HC164, 74HC4094 | Serial (DATA, CLOCK, LATCH) |## Quick Start

| DirectLatchDriver | 74HC373, 74HC75 | Parallel (8 GPIO pins) |

### Example 1: 74HC595 Shift Register

## Installation```cpp

#include <LatchController.h>

### PlatformIO#include <drivers/ShiftRegisterDriver.h>

```ini

lib_deps = // Create driver (DATA=23, CLOCK=18, LATCH=19)

    https://github.com/MROutake/Platform-IO-LIBS.git#LatchControllerShiftRegisterDriver driver(23, 18, 19);

```

// Create controller with 8 channels

## Quick StartLatchController latch(&driver, 8);



### 74HC595 Shift Register with Relay Modulevoid setup() {

  Serial.begin(115200);

```cpp  latch.begin(ACTIVE_LOW);  // For relay modules

#include <LatchController.h>}

#include <drivers/ShiftRegisterDriver.h>

void loop() {

// Hardware: 74HC595  latch.setLatchOn(0);   // Turn on channel 0

// DATA=23, CLOCK=18, LATCH=19  delay(500);

ShiftRegisterDriver driver(23, 18, 19);  latch.setLatchOff(0);

LatchController relays(&driver, 8);  delay(500);

}

void setup() {```

    Serial.begin(115200);

    ### Example 2: 74HC373 Direct D-Latch

    // ACTIVE_LOW for relay modules (LOW = relay ON)```cpp

    relays.begin(ACTIVE_LOW);#include <LatchController.h>

    #include <drivers/DirectLatchDriver.h>

    // All relays off at startup

    relays.setAllOff();// Define data pins (D0-D7)

}uint8_t dataPins[] = {23, 22, 21, 19, 18, 5, 17, 16};



void loop() {// Create driver (data pins, enable pin, channel count)

    relays.setLatchOn(0);   // Turn relay 0 ONDirectLatchDriver driver(dataPins, 4, 8);

    delay(1000);

    relays.setLatchOff(0);  // Turn relay 0 OFFLatchController latch(&driver, 8);

    delay(1000);

}void setup() {

```  latch.begin(ACTIVE_HIGH);

}

## API Reference```



### Initialization## API Reference



```cpp```cpp

LatchController(LatchDriver* driver, uint8_t channels = 8);// Init

bool begin(LatchTriggerMode mode = ACTIVE_HIGH);bool begin(LatchTriggerMode mode = ACTIVE_HIGH);

bool isInitialized();

```// Single latch control

bool setLatch(uint8_t channel, bool state);

### Single Channel Controlbool setLatchOn(uint8_t channel);

bool setLatchOff(uint8_t channel);

```cppbool toggleLatch(uint8_t channel);

bool setLatch(uint8_t channel, bool state);   // Set specific state

bool setLatchOn(uint8_t channel);             // Turn ON (logical)// Group control

bool setLatchOff(uint8_t channel);            // Turn OFF (logical)void setAllLatches(uint32_t mask);

bool toggleLatch(uint8_t channel);            // Toggle statevoid setAllOn();

```void setAllOff();



### Bulk Control// Status

bool getLatchState(uint8_t channel);

```cppuint32_t getAllStates();

void setAllLatches(uint32_t mask);  // Set all from bitmaskuint8_t getChannelCount();

void setAllOn();                    // All channels ON```

void setAllOff();                   // All channels OFF

```## Supported ICs



### State Query| IC | Type | Channels | Driver |

|----|------|----------|--------|

```cpp| 74HC595 | Shift Register | 8 (cascadable) | ShiftRegisterDriver |

bool getLatchState(uint8_t channel);  // Get single channel state| 74HC164 | Shift Register | 8 | ShiftRegisterDriver |

uint32_t getAllStates();              // Get all states as bitmask| 74HC373 | D-Latch (parallel) | 8 | DirectLatchDriver |

uint8_t getChannelCount();            // Get number of channels| 74HC75 | D-Latch | 4 | DirectLatchDriver |

```| CD4042 | D-Latch | 4 | DirectLatchDriver |



### Configuration## Creating Custom Drivers



```cppImplement the `LatchDriver` interface:

void setTriggerMode(LatchTriggerMode mode);  // ACTIVE_HIGH or ACTIVE_LOW

void printDebugInfo();                        // Print status to Serial```cpp

```class MyCustomDriver : public LatchDriver {

public:

## Trigger Modes    bool init() override {

        // Setup hardware

| Mode | Description | Use Case |        return true;

|------|-------------|----------|    }

| `ACTIVE_HIGH` | HIGH signal = device active | LEDs, most digital outputs |    

| `ACTIVE_LOW` | LOW signal = device active | Relay modules, optocouplers |    void updateHardware(uint32_t data, uint8_t channelCount) override {

        // Write data to hardware

**Important:** `setLatchOn()` and `setLatchOff()` always represent the **logical state**. Hardware inversion is handled automatically when `ACTIVE_LOW` is configured.    }

    

## FreeRTOS Best Practices    const char* getName() override {

        return "My Custom IC";

This library is designed for ESP32 dual-core architecture:    }

    

- **Core 1**: Run GPIO/Hardware tasks (including this library)    uint8_t getMaxChannels() override {

- **Core 0**: Run Network/WiFi tasks        return 8;

    }

```cpp};

// Relay Task on Core 1```

void relayTask(void* param) {

    relays.begin(ACTIVE_LOW);## License

    MIT

    for (;;) {
        // Hardware control here
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup() {
    xTaskCreatePinnedToCore(relayTask, "RelayTask", 4096, NULL, 1, NULL, 1);
}
```

## Creating Custom Drivers

Implement the `LatchDriver` interface:

```cpp
class MyCustomDriver : public LatchDriver {
public:
    bool init() override {
        // Initialize hardware
        return true;
    }
    
    void updateHardware(uint32_t data, uint8_t channelCount) override {
        // Send data to hardware
        // Note: data is already inverted if ACTIVE_LOW
    }
    
    const char* getName() override {
        return "My Custom Driver";
    }
    
    uint8_t getMaxChannels() override {
        return 8;
    }
};
```

## Version History

- **v3.0.0** - Professional refactor, fixed ACTIVE_LOW logic, English documentation
- **v2.0.0** - Added driver architecture, FreeRTOS support
- **v1.0.0** - Initial release

## License

MIT License - See LICENSE file for details.
