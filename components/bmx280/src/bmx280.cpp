#include "bmx280.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "math.h"


#define REG_CALIB_TEMP_PRESS  (uint8_t)0x88
#define REG_CALIB_PRESS       (uint8_t)0x8E
#define REG_CALIB_HUM         (uint8_t)0xA1
#define REG_CALIB_HUM_2       (uint8_t)0xE1

#define REG_ID          (uint8_t)0xD0
#define REG_RESET       (uint8_t)0xE0
#define REG_CTRL_HUM    (uint8_t)0xF2
#define REG_STATUS      (uint8_t)0xF3
#define REG_CTRL_MEAS   (uint8_t)0xF4
#define REG_CONFIG      (uint8_t)0xF5
#define REG_PRESSURE    (uint8_t)0xF7
#define REG_TEMPERATURE (uint8_t)0xFA
#define REG_HUMIDITY    (uint8_t)0xFD

#define RESET_VALUE          (uint8_t) 0xB6
#define STATUS_MEASURING_BIT (uint8_t) 0x08
#define STATUS_IM_UPDATE_BIT (uint8_t) 0x01

#define STATUS_MEASURING_TIMEOUT (uint16_t)500
#define STATUS_UPDATE_TIMEOUT   (uint16_t)100

#define FLOAT(value)  (static_cast<float>   (value))
#define INT64(value)  (static_cast<int64_t> (value))
#define INT32(value)  (static_cast<int32_t> (value))
#define UINT16(value) (static_cast<uint16_t>(value))
#define INT16(value)  (static_cast<int16_t> (value))
#define UINT8(value)  (static_cast<uint8_t> (value))
#define INT8(value)   (static_cast<int8_t>  (value))

static const char *TAG = "esp_cxx::bmx280";

namespace esp_cxx {

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

esp_err_t Bmx280::set_mode(const Mode mode)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t ctrl_meas = ((UINT8(m_config.osrs_t) & 0x07) << 5) |
                        ((UINT8(m_config.osrs_p) & 0x07) << 2) |
                        (UINT8(mode) & 0x03);
    esp_err_t err = write_reg(REG_CTRL_MEAS, ctrl_meas);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "failed to set mode");
        return err;
    }
    m_config.mode = mode;
    return ESP_OK;
}

esp_err_t Bmx280::get_mode(Mode &mode)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t ctrl_meas;
    esp_err_t err = read_reg(REG_CTRL_MEAS, ctrl_meas);
    if(err != ESP_OK) return err;
    mode = static_cast<Mode>(ctrl_meas & 0x03);
    m_config.mode = mode;
    return ESP_OK;
}

esp_err_t Bmx280::soft_reset()
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = write_reg(REG_RESET, RESET_VALUE);
    if(err != ESP_OK){
        ESP_LOGE(TAG,"failed in soft reset");
        return err;
    }

    uint16_t count_timeout = 0;
    do{
        if(count_timeout >= STATUS_UPDATE_TIMEOUT) break;
        vTaskDelay(pdMS_TO_TICKS(10));
        count_timeout += 10;
    }
    while(is_update());
    return err;
}

esp_err_t Bmx280::set_config(const Config &config)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = write_reg(REG_CTRL_MEAS, 0x00);
    if(err != ESP_OK) return err;
    
    err = soft_reset();
    if(err != ESP_OK) return err;

    if(m_model == Model::BMX280_MODEL_BME280){
        uint8_t ctrl_hum = UINT8(config.osrs_h) & 0x07;
        err = write_reg(REG_CTRL_HUM, ctrl_hum);
        if(err != ESP_OK) return err;
    }

    uint8_t cfg = ((UINT8(config.standby) & 0x07) << 5) |
                  ((UINT8(config.filter)  & 0x07) << 2) |
                  0x00;
    err = write_reg(REG_CONFIG, cfg);
    if(err != ESP_OK) return err;

    uint8_t ctrl_meas = ((UINT8(config.osrs_t) & 0x07) << 5) |
                        ((UINT8(config.osrs_p) & 0x07) << 2) |
                        (UINT8(config.mode) & 0x03);
    err = write_reg(REG_CTRL_MEAS, ctrl_meas);
    if(err != ESP_OK) return err;
    m_config = config;
    return ESP_OK;
}

esp_err_t Bmx280::get_config(Config &config)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_OK;

    if(m_model == Model::BMX280_MODEL_BME280){
        uint8_t ctrl_hum;
        err = read_reg(REG_CTRL_HUM, ctrl_hum);
        if(err != ESP_OK) return err;
        config.osrs_h = static_cast<Oversampling>(ctrl_hum & 0x07);
    }else{
        config.osrs_h = Oversampling::SKIP;
    }

    uint8_t cfg;
    err = read_reg(REG_CONFIG, cfg);
    if(err != ESP_OK) return err;
    config.standby = static_cast<Standby>(cfg >> 5);
    config.filter  = static_cast<IirFilter>((cfg >> 2) & 0x07);

    uint8_t ctrl_meas;
    err = read_reg(REG_CTRL_MEAS, ctrl_meas);
    if(err != ESP_OK) return err;
    config.osrs_t = static_cast<Oversampling>(ctrl_meas >> 5);
    config.osrs_p = static_cast<Oversampling>((ctrl_meas >> 2) & 0x07);
    config.mode   = static_cast<Mode>(ctrl_meas & 0x03);

    m_config = config;
    return ESP_OK;
}

esp_err_t Bmx280::read(Data &data)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    RawData raw;
    esp_err_t err = read_raw(raw);
    if(err != ESP_OK) return err;

    data.temp = raw.temp;

    if(raw.adc_p == 0x80000){
        data.press = 0.0;
        data.alt = 0.0;
    }else{
        data.press = compensate_pressure(raw.adc_p);
        data.alt = calculate_altitude(data.press);
    }

    if(raw.adc_h == 0x8000){
        data.hum = 0.0;
    }else{
        data.hum = compensate_humidity(raw.adc_h);
    }
    return ESP_OK;
}

esp_err_t Bmx280::read_temperature(float &temp)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    RawData raw;
    esp_err_t err = read_raw(raw);
    if(err != ESP_OK) return err;
    temp = raw.temp;
    return ESP_OK;
}

esp_err_t Bmx280::read_pressure(float &press)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    RawData raw;
    esp_err_t err = read_raw(raw);
    if(err != ESP_OK) return err;

    if(raw.adc_p == 0x80000){
        ESP_LOGE(TAG, "pressure measurement was disabled");
        return ESP_ERR_INVALID_STATE;
    }
    press = compensate_pressure(raw.adc_p);

    return ESP_OK;
}

esp_err_t Bmx280::read_humidity(float &hum)
{
    if(m_model != Model::BMX280_MODEL_BME280){
        hum = 0.0;
        ESP_LOGE(TAG, "BMP280 model cannot measure humidity");
        return ESP_ERR_NOT_SUPPORTED;
    }

    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    RawData raw;
    esp_err_t err = read_raw(raw);
    if(err != ESP_OK) return err;

    if(raw.adc_h == 0x8000){
        ESP_LOGE(TAG, "humidity measurement was disabled");
        return ESP_ERR_INVALID_STATE;
    }
    hum = compensate_humidity(raw.adc_h);
    return ESP_OK;
}

esp_err_t Bmx280::read_altitude(float &alt)
{
    if(!is_valid()){
        ESP_LOGE(TAG, "Bmx280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    float press;
    esp_err_t err = read_pressure(press);
    if(err != ESP_OK)return err;
    alt = calculate_altitude(press);
    return ESP_OK;
}


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

bool Bmx280::is_measuring()
{
    uint8_t status;
    if(read_reg(REG_STATUS, status) != ESP_OK){
        ESP_LOGE(TAG, "failed to read chip status");
        return false;
    } 
    return status & STATUS_MEASURING_BIT;
}

bool Bmx280::is_update()
{
    uint8_t status;
    if(read_reg(REG_STATUS, status) != ESP_OK){
        ESP_LOGE(TAG, "failed to read chip status");
        return false;
    }
    return status & STATUS_IM_UPDATE_BIT;
}

esp_err_t Bmx280::read_calibration()
{
    uint8_t buf[24];
    esp_err_t err = read_regs(REG_CALIB_TEMP_PRESS, buf, sizeof(buf));
    if(err != ESP_OK) return err;
    m_calib.t.dig_1 = UINT16((buf[1] << 8) | buf[0]);
    m_calib.t.dig_2 = INT16 ((buf[3] << 8) | buf[2]);
    m_calib.t.dig_3 = INT16 ((buf[5] << 8) | buf[4]);

    m_calib.p.dig_1 = UINT16((buf[7]  << 8) | buf[6]);
    m_calib.p.dig_2 = INT16 ((buf[9]  << 8) | buf[8]);
    m_calib.p.dig_3 = INT16 ((buf[11] << 8) | buf[10]);
    m_calib.p.dig_4 = INT16 ((buf[13] << 8) | buf[12]);
    m_calib.p.dig_5 = INT16 ((buf[15] << 8) | buf[14]);
    m_calib.p.dig_6 = INT16 ((buf[17] << 8) | buf[16]);
    m_calib.p.dig_7 = INT16 ((buf[19] << 8) | buf[18]);
    m_calib.p.dig_8 = INT16 ((buf[21] << 8) | buf[20]);
    m_calib.p.dig_9 = INT16 ((buf[23] << 8) | buf[22]);

    if(m_model == Model::BMX280_MODEL_BME280){

        err = read_reg(REG_CALIB_HUM, m_calib.h.dig_1);
        if(err != ESP_OK) return err;
        uint8_t hbuf[7];

        err = read_regs(REG_CALIB_HUM_2, hbuf, sizeof(hbuf));
        if(err != ESP_OK) return err;

        m_calib.h.dig_2 = INT16((hbuf[1] << 8) | hbuf[0]);
        m_calib.h.dig_3 = hbuf[2];
        m_calib.h.dig_4 = INT16((hbuf[3] << 4) | (hbuf[4] & 0x0F));
        m_calib.h.dig_5 = INT16((hbuf[5] << 4) | (hbuf[4] >> 4));
        m_calib.h.dig_6 = INT8 (hbuf[6]);
    }   

    return ESP_OK;
}

esp_err_t Bmx280::read_raw(RawData &raw)
{
    esp_err_t err = ESP_OK;

    if(m_config.mode != Mode::NORMAL){

        err = set_mode(Mode::FORCED);
        if(err != ESP_OK) return err;

        uint16_t count_timeout = 0;
        do{
            if(count_timeout >= STATUS_MEASURING_TIMEOUT) break;
            vTaskDelay(pdMS_TO_TICKS(10));
            count_timeout += 10;
        }
        while(is_measuring());
    }

    uint8_t buf[8];
    err = read_regs(REG_PRESSURE, buf, sizeof(buf));
    if(err != ESP_OK) return err;

    raw.adc_p = (INT32(buf[0]) << 12) | (INT32(buf[1]) << 4) | (INT32(buf[2]) >> 4);
    raw.adc_t = (INT32(buf[3]) << 12) | (INT32(buf[4]) << 4) | (INT32(buf[5]) >> 4);

    if(m_model == Model::BMX280_MODEL_BME280){
        raw.adc_h = INT32((buf[6] << 8) | buf[7]);
    } else {
        raw.adc_h = 0x8000;
    }

    if(raw.adc_t == 0x80000){
        ESP_LOGE(TAG, "measurement was disabled");
        return ESP_ERR_INVALID_STATE;
    }

    raw.temp = compensate_temperature(raw.adc_t);

    return ESP_OK;
}

float Bmx280::compensate_temperature(int32_t adc_t)
{
    int32_t var1 = ((((adc_t >> 3) - (INT32(m_calib.t.dig_1) << 1)))
                    * INT32(m_calib.t.dig_2)) >> 11;

    int32_t var2 = (((((adc_t >> 4) - INT32(m_calib.t.dig_1))
                    * ((adc_t >> 4) - INT32(m_calib.t.dig_1))) >> 12)
                    * INT32(m_calib.t.dig_3)) >> 14;

    m_t_fine = var1 + var2;

    int32_t T = (m_t_fine * 5 + 128) >> 8;

    return FLOAT(T) / 100.0f;
}

float Bmx280::compensate_pressure(int32_t adc_p) const
{
    int64_t var1, var2, p;

    var1 = INT64(m_t_fine) - 128000;

    var2 = var1 * var1 * INT64(m_calib.p.dig_6);
    var2 = var2 + ((var1 * INT64(m_calib.p.dig_5)) << 17);
    var2 = var2 + (INT64(m_calib.p.dig_4) << 35);

    var1 = ((var1 * var1 * INT64(m_calib.p.dig_3)) >> 8)
         + ((var1 * INT64(m_calib.p.dig_2)) << 12);
    var1 = ((INT64(1) << 47) + var1)
         * INT64(m_calib.p.dig_1) >> 33;

    if(var1 == 0){
        ESP_LOGE(TAG, "compensate pressure ERROR");
        return 0.0f;
    }
    p = 1048576 - INT64(adc_p);
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (INT64(m_calib.p.dig_9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (INT64(m_calib.p.dig_8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (INT64(m_calib.p.dig_7) << 4);
    return FLOAT(p) / 256.0f;
}


float Bmx280::compensate_humidity(int32_t adc_h) const
{
    int32_t v = m_t_fine - 76800;

    v = ((((adc_h << 14)
        - (INT32(m_calib.h.dig_4) << 20)
        - (INT32(m_calib.h.dig_5) * v)) + 16384) >> 15)
        * (((((((v * INT32(m_calib.h.dig_6)) >> 10)
        * (((v * INT32(m_calib.h.dig_3)) >> 11) + 32768)) >> 10) + 2097152)
        * INT32(m_calib.h.dig_2) + 8192) >> 14);

    v = v - (((((v >> 15) * (v >> 15)) >> 7)
        * INT32(m_calib.h.dig_1)) >> 4);

    v = (v < 0) ? 0 : v;
    v = (v > 419430400) ? 419430400 : v;

    return FLOAT(v >> 12) / 1024.0f;
}

float Bmx280::calculate_altitude(float press) const
{
    return 44330.0 * (1.0 - pow(press / m_sea_level_PA, 0.1903));
}
} // namespace esp_cxx
