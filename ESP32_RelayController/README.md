# ESP32_RelayController

Thread-safe library for controlling 8 relays via 74HC595 shift register.

## Features
- ✅ FreeRTOS mutex for multi-core safety
- ✅ Configurable pins (GPIO23/18/19 default)
- ✅ HIGH/LOW trigger modes
- ✅ Individual & group relay control

## Installation

### PlatformIO
```ini
lib_deps = 
    https://github.com/YOUR_USERNAME/Platform_IO_Libs.git#ESP32_RelayController
```

Or as git submodule:
```bash
git submodule add https://github.com/YOUR_USERNAME/Platform_IO_Libs.git lib/Platform_IO_Libs
```

## Usage
```cpp
#include "ESP32_RelayController.h"

ESP32_RelayController relay;

void setup() {
  // Init: DATA=23, CLOCK=18, LATCH=19, OE=none, LOW_TRIGGER
  relay.begin(23, 18, 19, 0xFF, LOW_TRIGGER);
}

void loop() {
  relay.setRelayOn(0);
  delay(500);
  relay.setRelayOff(0);
  delay(500);
}
```

## Hardware
```
ESP32 DevKit → 74HC595 → Relay Module
GPIO 23 → DS (14)
GPIO 18 → SHCP (11)
GPIO 19 → STCP (12)
GND → GND (8)
3.3V → VCC (16)
      MR (10) → VCC
```

## License
MIT
