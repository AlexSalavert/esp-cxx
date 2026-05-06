#include "bmx280.hpp"

#include "esp_log.h"


#define REG_CALIB_TEMP_PRESS  (uint8_t)0x88
#define REG_CALIB_PRESS       (uint8_t)0x8E
#define REG_CALIB_HUM         (uint8_t)0xA1
#define REG_CALIB_HUM_2       (uint8_t)0xE1

#define REG_ID          (uint8_t)0xD0
#define REG_RESET       (uint8_t)0xE0
#define REG_STATUS      (uint8_t)0xF3
#define REG_CONFIG      (uint8_t)0xF5
#define REG_TEMPERATURE (uint8_t)0xFA // FA - FC
#define REG_PRESSURE    (uint8_t)0xF7 // F7 - F9
#define REG_HUMIDITY    (uint8_t)0xFD // FD - FE

static const char *TAG = "esp_cxx::bmx280";

namespace esp_cxx {

esp_err_t Bmx280::write_reg(const uint8_t reg, const uint8_t value)
{
    uint8_t buf[] = {reg, value};
    return m_dev->write(buf, sizeof(buf));
}


esp_err_t Bmx280::read_reg(const uint8_t reg, uint8_t &value)
{
    return m_dev->write_read(&reg, sizeof(reg), &value, sizeof(value));
}

esp_err_t Bmx280::read_regs(const uint8_t reg, uint8_t *buf, size_t len)
{
    return m_dev->write_read(&reg, sizeof(reg), buf, len);
}

esp_err_t Bmx280::read_calibration()
{
    uint8_t buf[24];
    esp_err_t err = read_regs(REG_CALIB_TEMP_PRESS, buf, sizeof(buf));
    if(err != ESP_OK) return err;
    m_calib.t.dig_1 = static_cast<uint16_t> ((buf[1] << 8) | buf[0]);
    m_calib.t.dig_2 = static_cast<int16_t>  ((buf[3] << 8) | buf[2]);
    m_calib.t.dig_3 = static_cast<int16_t>  ((buf[5] << 8) | buf[4]);

    m_calib.p.dig_1 = static_cast<uint16_t> ((buf[7]  << 8) | buf[6]);
    m_calib.p.dig_2 = static_cast<int16_t>  ((buf[9]  << 8) | buf[8]);
    m_calib.p.dig_3 = static_cast<int16_t>  ((buf[11] << 8) | buf[10]);
    m_calib.p.dig_4 = static_cast<int16_t>  ((buf[13] << 8) | buf[12]);
    m_calib.p.dig_5 = static_cast<int16_t>  ((buf[15] << 8) | buf[14]);
    m_calib.p.dig_6 = static_cast<int16_t>  ((buf[17] << 8) | buf[16]);
    m_calib.p.dig_7 = static_cast<int16_t>  ((buf[19] << 8) | buf[18]);
    m_calib.p.dig_8 = static_cast<int16_t>  ((buf[21] << 8) | buf[20]);
    m_calib.p.dig_9 = static_cast<int16_t>  ((buf[23] << 8) | buf[22]);

    if(m_model == Model::BMX280_MODEL_BME280){

        err = read_reg(REG_CALIB_HUM, m_calib.h.dig_1);
        if(err != ESP_OK) return err;
        uint8_t hbuf[7];

        err = read_regs(REG_CALIB_HUM_2, hbuf, sizeof(hbuf));
        if(err != ESP_OK) return err;

        m_calib.h.dig_2 = static_cast<int16_t> ((hbuf[1] << 8) | hbuf[0]);
        m_calib.h.dig_3 = hbuf[2];
        m_calib.h.dig_4 = static_cast<int16_t> ((hbuf[3] << 4) | (hbuf[4] & 0x0F));
        m_calib.h.dig_5 = static_cast<int16_t> ((hbuf[5] << 4) | (hbuf[4] >> 4));
        m_calib.h.dig_6 = static_cast<int8_t>  (hbuf[6]);
    }   

    return ESP_OK;
}

Bmx280::Bmx280(const I2cMaster& bus, uint8_t addr)
{
    if(!bus.is_valid()){
        ESP_LOGE(TAG, "i2c bus not valid");
        return;
    }

    if(addr != BMX280_I2C_ADDR_LOW && addr != BMX280_I2C_ADDR_HIGH){
        ESP_LOGE(TAG, "Invalid I2C address 0x%02X", addr);
        return;
    }

    if(bus.probe(addr) != ESP_OK){
        ESP_LOGE(TAG, "BMx280 not found at address 0x%02X", addr);
        return;
    }

    m_dev.emplace(bus, I2cDevice::Config{
        .address      = addr,
        .scl_speed_hz = 400000,
        .timeout_ms   = 100,
    });

    uint8_t id;
    if(read_reg(REG_ID, id) != ESP_OK){
        ESP_LOGE(TAG,"failed to read chip ID");
        m_dev.reset();
        return;
    }

    if(id != BMX280_CHIP_ID_BME280 && id != BMX280_CHIP_ID_BMP280){
        ESP_LOGE(TAG, "BMx280 chip id invalid 0x%2X", id);
        m_dev.reset();
        return;
    }

    m_model = static_cast<Model>(id);

    if(read_calibration() != ESP_OK){
        ESP_LOGE(TAG, "failed to read chip calibration");
        m_dev.reset();
        return;
    }

}
Bmx280::~Bmx280()
{

}

esp_err_t Bmx280::read_temperature(float &temp)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buf[3];
    esp_err_t err = read_regs(REG_TEMPERATURE, buf, sizeof(buf));
    if(err != ESP_OK) return err;

    int32_t adc_t = (static_cast<int32_t>(buf[0]) << 12) |
                    (static_cast<int32_t>(buf[1]) << 4)  |
                    (static_cast<int32_t>(buf[2]) >> 4);
    if(adc_t == 0x80000){
        ESP_LOGE(TAG, "temperature measurement was disabled");
        return ESP_ERR_INVALID_STATE;
    }
    temp = compensate_temperature(adc_t);
    return err;
}

esp_err_t Bmx280::read_pressure(float &press)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    float temp;
    esp_err_t err = read_temperature(temp);
    if(err != ESP_OK) return err;

    uint8_t buf[3];
    err = read_regs(REG_PRESSURE, buf, sizeof(buf));
    if(err != ESP_OK) return err;

    int32_t adc_p = (static_cast<int32_t>(buf[0]) << 12) |
                    (static_cast<int32_t>(buf[1]) << 4)  |
                    (static_cast<int32_t>(buf[2]) >> 4);
    if(adc_p == 0x80000){
        ESP_LOGE(TAG, "pressure measurement was disabled");
        return ESP_ERR_INVALID_STATE;
    }
    press = compensate_pressure(adc_p);

    return ESP_OK;
}

float Bmx280::compensate_temperature(int32_t adc_t)
{
    int32_t var1 = ((((adc_t >> 3) - (static_cast<int32_t>(m_calib.t.dig_1) << 1)))
                    * static_cast<int32_t>(m_calib.t.dig_2)) >> 11;

    int32_t var2 = (((((adc_t >> 4) - static_cast<int32_t>(m_calib.t.dig_1))
                    * ((adc_t >> 4) - static_cast<int32_t>(m_calib.t.dig_1))) >> 12)
                    * static_cast<int32_t>(m_calib.t.dig_3)) >> 14;

    m_t_fine = var1 + var2;

    int32_t T = (m_t_fine * 5 + 128) >> 8;

    return static_cast<float>(T) / 100.0f;
}

float Bmx280::compensate_pressure(int32_t adc_p) const
{
    int64_t var1, var2, p;

    var1 = static_cast<int64_t>(m_t_fine) - 128000;

    var2 = var1 * var1 * static_cast<int64_t>(m_calib.p.dig_6);
    var2 = var2 + ((var1 * static_cast<int64_t>(m_calib.p.dig_5)) << 17);
    var2 = var2 + (static_cast<int64_t>(m_calib.p.dig_4) << 35);

    var1 = ((var1 * var1 * static_cast<int64_t>(m_calib.p.dig_3)) >> 8)
         + ((var1 * static_cast<int64_t>(m_calib.p.dig_2)) << 12);
    var1 = ((static_cast<int64_t>(1) << 47) + var1)
         * static_cast<int64_t>(m_calib.p.dig_1) >> 33;

    if(var1 == 0){
        ESP_LOGE(TAG, "compensate pressure ERROR");
        return 0.0f;
    }
    p = 1048576 - static_cast<int64_t>(adc_p);
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (static_cast<int64_t>(m_calib.p.dig_9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (static_cast<int64_t>(m_calib.p.dig_8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (static_cast<int64_t>(m_calib.p.dig_7) << 4);
    return static_cast<float>(p) / 256.0f;
}
} // namespace esp_cxx