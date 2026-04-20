# esp-cxx

C++ components for ESP-IDF.

## Requirements

- ESP-IDF >= 5.0
- C++17 or later

## Components

| Component | Description |
|---|---|
| [timer](components/timer) | ESP timer wrappers with RAII and lambda support: `PeriodicTimer` for recurring callbacks, `OneShotTimer` for one-time callbacks |
| [status_led](components/status_led) | Status LED abstraction with a common `StatusLed` base class: `SimpleLed` for GPIO-based LEDs, `Ws2812Led` for RMT-based WS2812 strips with per-pixel color control |

## Usage

Clone as a submodule inside your project's `components/` folder:

    git submodule add https://github.com/AlexSalavert/esp-cxx components/esp-cxx

Then in your project's `CMakeLists.txt`:

    set(EXTRA_COMPONENT_DIRS "components/esp-cxx/components")

## License

MIT