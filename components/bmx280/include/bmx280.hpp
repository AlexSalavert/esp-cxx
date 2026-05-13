#pragma once
#include <optional>
#include "i2c.hpp"

namespace esp_cxx {
static constexpr uint8_t BMX280_I2C_ADDR_LOW  = 0x76; ///< SDO → GND
static constexpr uint8_t BMX280_I2C_ADDR_HIGH = 0x77; ///< SDO → VCC

static constexpr uint8_t BMX280_CHIP_ID_BMP280 = 0x58;
static constexpr uint8_t BMX280_CHIP_ID_BME280 = 0x60;

class Bmx280{
public:

    enum class Model : uint8_t {
        BMX280_MODEL_UNKNOWN = 0,
        BMX280_MODEL_BME280  = 0x60,
        BMX280_MODEL_BMP280  = 0x58,
    };

    enum class Oversampling : uint8_t {
        SKIP = 0x00, // Measurement disabled
        X1   = 0x01, // 16 bit
        X2   = 0x02, // 17 bit
        X4   = 0x03, // 18 bit
        X8   = 0x04, // 19 bit
        X16  = 0x05, // 20 bit — maximum resolution
    };

    enum class IirFilter : uint8_t {
        OFF = 0x00, // No filter — more noise, less latency
        X2  = 0x01,
        X4  = 0x02,
        X8  = 0x03,
        X16 = 0x04, // Maximum smoothing — ideal for indoors
    };

    enum class Mode : uint8_t {
        SLEEP  = 0x00, // Without measurements, minimal consumption
        FORCED = 0x01, // One measure, then return to SLEEP
        NORMAL = 0x03, // Continuous measurements with standby in between
    };

    enum class Standby : uint8_t {
        MS_0_5  = 0x00, // 0.5 ms 
        MS_62_5 = 0x01, // 62.5 ms
        MS_125  = 0x02, // 125 ms 
        MS_250  = 0x03, // 250 ms 
        MS_500  = 0x04, // 500 ms 
        MS_1000 = 0x05, // 1000 ms

        BME280_MS_10   = 0x06, // 10 ms   — only BME280
        BME280_MS_20   = 0x07, // 20 ms   — only BME280

        BMP280_MS_2000 = 0x06, // 2000 ms — only BMP280
        BMP280_MS_4000 = 0x07, // 4000 ms — only BMP280
    };

    struct Config {
        Oversampling osrs_t  = Oversampling::X2;
        Oversampling osrs_p  = Oversampling::X16;
        Oversampling osrs_h  = Oversampling::X1;
        IirFilter    filter  = IirFilter::X16;
        Mode         mode    = Mode::NORMAL;
        Standby      standby = Standby::MS_1000;

        static Config weatherMonitoring() {
            return {
                .osrs_t  = Oversampling::X1,
                .osrs_p  = Oversampling::X1,
                .osrs_h  = Oversampling::X1,
                .filter  = IirFilter::OFF,
                .mode    = Mode::FORCED,
                .standby = Standby::MS_1000,
            };
        }

        static Config humiditySensing() {
            return {
                .osrs_t  = Oversampling::X1,
                .osrs_p  = Oversampling::SKIP,
                .osrs_h  = Oversampling::X1,
                .filter  = IirFilter::OFF,
                .mode    = Mode::FORCED,
                .standby = Standby::MS_1000,
            };
        }

        static Config indoorNavigation() {
            return {
                .osrs_t  = Oversampling::X2,
                .osrs_p  = Oversampling::X16,
                .osrs_h  = Oversampling::X1,
                .filter  = IirFilter::X16,
                .mode    = Mode::NORMAL,
                .standby = Standby::MS_0_5,
            };
        }

        static Config gaming() {
            return {
                .osrs_t  = Oversampling::X1,
                .osrs_p  = Oversampling::X4,
                .osrs_h  = Oversampling::SKIP,
                .filter  = IirFilter::X16,
                .mode    = Mode::NORMAL,
                .standby = Standby::MS_0_5,
            };
        }

        static Config highPrecision() {
            return {
                .osrs_t  = Oversampling::X16,
                .osrs_p  = Oversampling::X16,
                .osrs_h  = Oversampling::X16,
                .filter  = IirFilter::X16,
                .mode    = Mode::NORMAL,
                .standby = Standby::MS_500,
            };
        }
    };

    struct Data {
        float temp;
        float press;
        float hum;
        float alt;
    };

    explicit Bmx280(const I2cMaster& bus, uint8_t addr);
    ~Bmx280() = default;

    // non copyable, non movable
    Bmx280(const Bmx280&)             = delete;
    Bmx280& operator=(const Bmx280&)  = delete;
    Bmx280(const Bmx280&&)            = delete;
    Bmx280& operator=(const Bmx280&&) = delete;

    esp_err_t soft_reset();

    esp_err_t set_config(const Config &config);
    esp_err_t get_config(Config &config);

    esp_err_t set_mode(const Mode mode);
    esp_err_t get_mode(Mode &mode);

    esp_err_t read(Data &data);
    esp_err_t read_temperature(float &temp);
    esp_err_t read_pressure(float &press);
    esp_err_t read_humidity(float &hum);
    esp_err_t read_altitude(float &alt);

    void set_sea_level_pressure(float sea_level_PA = 101325.0f) {m_sea_level_PA = sea_level_PA;}

    bool is_valid() const { return m_dev.has_value(); }
    Model model() const { return m_model; }
    
private:

    struct CalibTemp {
        uint16_t dig_1;
        int16_t  dig_2;
        int16_t  dig_3;
    };

    struct CalibPress {
        uint16_t dig_1;
        int16_t  dig_2;
        int16_t  dig_3;
        int16_t  dig_4;
        int16_t  dig_5;
        int16_t  dig_6;
        int16_t  dig_7;
        int16_t  dig_8;
        int16_t  dig_9;
    };

    struct CalibHum {
        uint8_t  dig_1;
        int16_t  dig_2;
        uint8_t  dig_3;
        int16_t  dig_4;
        int16_t  dig_5;
        int8_t   dig_6;
    };

    struct CalibData {
        CalibTemp  t;
        CalibPress p;
        CalibHum   h;
    };

    struct RawData {
        int32_t adc_t;
        int32_t adc_p;
        int32_t adc_h;
        float   temp;
    };

    std::optional<I2cDevice> m_dev;
    Model                    m_model;
    int32_t                  m_t_fine;
    CalibData                m_calib;
    Config                   m_config;
    float                    m_sea_level_PA = 101325.0f;

    esp_err_t write_reg(const uint8_t reg, const uint8_t value);
    esp_err_t read_reg(const uint8_t reg, uint8_t &value);
    esp_err_t read_regs(const uint8_t reg, uint8_t *buf, size_t len);

    bool is_measuring();
    bool is_update();
    esp_err_t read_calibration();

    esp_err_t read_raw(RawData &raw);

    float compensate_temperature(int32_t adc_t);
    float compensate_pressure(int32_t adc_p) const;
    float compensate_humidity(int32_t adc_h) const;
    float calculate_altitude(float press) const;
};
} // namespace esp_cxx
