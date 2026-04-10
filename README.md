# esp-cxx

C++ components for ESP-IDF.

## Requirements

- ESP-IDF >= 5.0
- C++17 or later

## Components

| Component | Description |
|---|---|
| [timer](components/timer) | Periodic timer with RAII and lambda support |

## Usage

Clone as a submodule inside your project's `components/` folder:

    git submodule add https://github.com/tuusuario/esp-cxx components/esp-cxx

Then in your project's `CMakeLists.txt`:

    set(EXTRA_COMPONENT_DIRS "components/esp-cxx/components")

## License

MIT