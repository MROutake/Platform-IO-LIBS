# Platform_IO_Libs

Private collection of reusable PlatformIO libraries for ESP32 projects.

## Libraries

### ESP32_RelayController
Thread-safe relay control via 74HC595 shift register with FreeRTOS support.
- **Version:** 1.0.0
- **Platform:** ESP32
- **Features:** Multi-core safe, configurable pins, HIGH/LOW trigger modes

## Usage in Projects

### Method 1: Git Submodule (Recommended)
```bash
cd your-project
git submodule add https://github.com/YOUR_USERNAME/Platform_IO_Libs.git lib/Platform_IO_Libs
```

Then in `platformio.ini`:
```ini
lib_deps = 
    file://lib/Platform_IO_Libs/ESP32_RelayController
```

### Method 2: Direct Git Dependency
```ini
lib_deps = 
    https://github.com/YOUR_USERNAME/Platform_IO_Libs.git#ESP32_RelayController
```

## Adding New Libraries
Each library should have:
- `LibraryName/LibraryName.h`
- `LibraryName/LibraryName.cpp`
- `LibraryName/library.json`
- `LibraryName/README.md`

## License
MIT
