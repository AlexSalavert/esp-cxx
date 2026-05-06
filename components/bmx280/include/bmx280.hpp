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

    enum class Oversampling : uint8_t {
        SKIP = 0x00, ///< Medida desactivada
        X1   = 0x01, ///< 16 bit de resolución
        X2   = 0x02, ///< 17 bit
        X4   = 0x03, ///< 18 bit
        X8   = 0x04, ///< 19 bit
        X16  = 0x05, ///< 20 bit — máxima resolución
    };

    enum class IirFilter : uint8_t {
        OFF = 0x00, ///< Sin filtro — mayor ruido, menor latencia
        X2  = 0x01,
        X4  = 0x02,
        X8  = 0x03,
        X16 = 0x04, ///< Máximo suavizado — ideal para indoor
    };

    enum class Mode : uint8_t {
        SLEEP  = 0x00, ///< Sin medidas, consumo mínimo
        FORCED = 0x01, ///< Una medida, luego vuelve a SLEEP
        NORMAL = 0x03, ///< Medidas continuas con standby entre ellas
    };

    enum class Standby : uint8_t {
        MS_0_5  = 0x00, ///< 0.5 ms  — válido en ambos sensores
        MS_62_5 = 0x01, ///< 62.5 ms — válido en ambos sensores
        MS_125  = 0x02, ///< 125 ms  — válido en ambos sensores
        MS_250  = 0x03, ///< 250 ms  — válido en ambos sensores
        MS_500  = 0x04, ///< 500 ms  — válido en ambos sensores
        MS_1000 = 0x05, ///< 1000 ms — válido en ambos sensores

        BME280_MS_10   = 0x06, ///< 10 ms   — solo BME280
        BME280_MS_20   = 0x07, ///< 20 ms   — solo BME280

        BMP280_MS_2000 = 0x06, ///< 2000 ms — solo BMP280
        BMP280_MS_4000 = 0x07, ///< 4000 ms — solo BMP280
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
                .mode    = Mode::NORMAL,
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

        static Config lowPowerForced() {
            return {
                .osrs_t  = Oversampling::X1,
                .osrs_p  = Oversampling::X1,
                .osrs_h  = Oversampling::X1,
                .filter  = IirFilter::OFF,
                .mode    = Mode::SLEEP,
                .standby = Standby::MS_1000,
            };
        }

        static Config highPrecision() {
            return {
                .osrs_t  = Oversampling::X16,
                .osrs_p  = Oversampling::X16,
                .osrs_h  = Oversampling::X16,
                .filter  = IirFilter::X16,
                .mode    = Mode::NORMAL,
                .standby = Standby::MS_125,
            };
        }
    };

    struct Data {
        float temp;
        float press;
        float hum;
    };

    explicit Bmx280(const I2cMaster& bus, uint8_t addr);
    ~Bmx280();

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
    esp_err_t read_altitude(float &alt, float sea_level_PA = 101325.0f);

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

    std::optional<I2cDevice> m_dev;
    Model                    m_model;
    int32_t                  m_t_fine;
    CalibData                m_calib;

    esp_err_t write_reg(const uint8_t reg, const uint8_t value);
    esp_err_t read_reg(const uint8_t reg, uint8_t &value);
    esp_err_t read_regs(const uint8_t reg, uint8_t *buf, size_t len);

    esp_err_t read_calibration();

    esp_err_t read_raw();
    float compensate_temperature(int32_t adc_t);
    float compensate_pressure(int32_t adc_p) const;
    float compensate_humidity(int32_t adc_h) const;

};
} // namespace esp_cxx