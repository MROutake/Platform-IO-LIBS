# Platform_IO_Libs

Private collection of reusable PlatformIO libraries for ESP32 projects.

## Libraries

### LatchController ⭐ NEW v2.0
Universal abstraction library for controlling latch ICs.
- **Version:** 2.0.0
- **Platform:** ESP32
- **Supported ICs:** 74HC595, 74HC373, 74HC164, 74HC75, CD4042
- **Features:** Driver architecture, thread-safe, extensible, up to 32 channels

### ESP32_RelayController (deprecated)
Legacy relay controller for 74HC595. Use `LatchController` with `ShiftRegisterDriver` instead.
- **Version:** 1.0.0
- **Status:** ⚠️ Replaced by LatchController

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
