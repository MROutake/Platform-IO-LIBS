# LatchController v2.0

🎯 **Universal abstraction library for controlling latches on ESP32**

Supports multiple IC types through exchangeable drivers:
- **Shift Registers:** 74HC595, 74HC164, 74HC4094
- **Direct D-Latches:** 74HC373, 74HC75, CD4042
- **Extensible:** Add your own driver for any latch-based IC

## Features
- ✅ Abstract API - same code for different ICs
- ✅ Thread-safe (FreeRTOS mutex)
- ✅ Support for up to 32 channels
- ✅ ACTIVE_HIGH / ACTIVE_LOW modes
- ✅ Individual & group control

## Quick Start

### Example 1: 74HC595 Shift Register
```cpp
#include <LatchController.h>
#include <drivers/ShiftRegisterDriver.h>

// Create driver (DATA=23, CLOCK=18, LATCH=19)
ShiftRegisterDriver driver(23, 18, 19);

// Create controller with 8 channels
LatchController latch(&driver, 8);

void setup() {
  Serial.begin(115200);
  latch.begin(ACTIVE_LOW);  // For relay modules
}

void loop() {
  latch.setLatchOn(0);   // Turn on channel 0
  delay(500);
  latch.setLatchOff(0);
  delay(500);
}
```

### Example 2: 74HC373 Direct D-Latch
```cpp
#include <LatchController.h>
#include <drivers/DirectLatchDriver.h>

// Define data pins (D0-D7)
uint8_t dataPins[] = {23, 22, 21, 19, 18, 5, 17, 16};

// Create driver (data pins, enable pin, channel count)
DirectLatchDriver driver(dataPins, 4, 8);

LatchController latch(&driver, 8);

void setup() {
  latch.begin(ACTIVE_HIGH);
}
```

## API Reference

```cpp
// Init
bool begin(LatchTriggerMode mode = ACTIVE_HIGH);

// Single latch control
bool setLatch(uint8_t channel, bool state);
bool setLatchOn(uint8_t channel);
bool setLatchOff(uint8_t channel);
bool toggleLatch(uint8_t channel);

// Group control
void setAllLatches(uint32_t mask);
void setAllOn();
void setAllOff();

// Status
bool getLatchState(uint8_t channel);
uint32_t getAllStates();
uint8_t getChannelCount();
```

## Supported ICs

| IC | Type | Channels | Driver |
|----|------|----------|--------|
| 74HC595 | Shift Register | 8 (cascadable) | ShiftRegisterDriver |
| 74HC164 | Shift Register | 8 | ShiftRegisterDriver |
| 74HC373 | D-Latch (parallel) | 8 | DirectLatchDriver |
| 74HC75 | D-Latch | 4 | DirectLatchDriver |
| CD4042 | D-Latch | 4 | DirectLatchDriver |

## Creating Custom Drivers

Implement the `LatchDriver` interface:

```cpp
class MyCustomDriver : public LatchDriver {
public:
    bool init() override {
        // Setup hardware
        return true;
    }
    
    void updateHardware(uint32_t data, uint8_t channelCount) override {
        // Write data to hardware
    }
    
    const char* getName() override {
        return "My Custom IC";
    }
    
    uint8_t getMaxChannels() override {
        return 8;
    }
};
```

## License
MIT
