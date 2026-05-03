#pragma once
#include <optional>
#include "i2c.hpp"

namespace esp_cxx {

class Bmx280{
public:
    enum class Addr : uint8_t { 
        BMX280_ADDR_UNKNOWN = 0,
        BMX280_ADDR_PRIM    = 0x76, 
        BMX280_ADDR_SEC     = 0x77 
    };

    enum class Model : uint8_t {
        BMX280_MODEL_UNKNOWN = 0,
        BMX280_MODEL_BME280  = 0x60,
        BMX280_MODEL_BMP280  = 0x58,
    };

    explicit Bmx280(const I2cMaster& bus, Addr address = Addr::BMX280_ADDR_UNKNOWN);
    ~Bmx280();

    // non copyable, non movable
    Bmx280(const Bmx280&)             = delete;
    Bmx280& operator=(const Bmx280&)  = delete;
    Bmx280(const Bmx280&&)            = delete;
    Bmx280& operator=(const Bmx280&&) = delete;

    bool is_valid() const { return m_dev.has_value(); }
    Model model() const { return m_model; }
private:
    std::optional<I2cDevice> m_dev;
    Model                    m_model = Model::BMX280_MODEL_UNKNOWN;
};
} // namespace esp_cxx