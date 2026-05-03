#include "bmx280.hpp"

#include "esp_log.h"

#define REG_ID    (uint8_t)0xD0
#define ID_BME280 (uint8_t)0x60
#define ID_BMP280 (uint8_t)0x58

static const char *TAG = "esp_cxx::bmx280";

namespace esp_cxx {
Bmx280::Bmx280(const I2cMaster& bus, Addr address)
{
    if(!bus.is_valid()){
        ESP_LOGE(TAG, "i2c bus not valid");
        return;
    }
    uint16_t address_select;
    if(address == Addr::BMX280_ADDR_UNKNOWN){
        if(bus.probe(static_cast<uint16_t>(Addr::BMX280_ADDR_PRIM)) == ESP_OK){
            address_select = static_cast<uint16_t>(Addr::BMX280_ADDR_PRIM);
        } else if(bus.probe(static_cast<uint16_t>(Addr::BMX280_ADDR_SEC)) == ESP_OK){
            address_select = static_cast<uint16_t>(Addr::BMX280_ADDR_SEC);
        } else {
            ESP_LOGE(TAG, "no BMx280 found on I2C bus");
            return;
        }
    } else {
        if(bus.probe(static_cast<uint16_t>(address)) == ESP_OK){
            address_select = static_cast<uint16_t>(address);
        } else {
            ESP_LOGE(TAG, "BMx280 not found at address 0x%02X", static_cast<uint8_t>(address));
            return;
        }
    }

    m_dev.emplace(bus, I2cDevice::Config{
        .address      = address_select,
        .scl_speed_hz = 400000,
        .timeout_ms   = 100,
    });
    uint8_t model_read = 0;
    uint8_t reg_id = REG_ID;
    if(m_dev->write_read(&reg_id, sizeof(reg_id), &model_read, sizeof(model_read)) != ESP_OK){
        ESP_LOGE(TAG, "failed to read chip ID");
        m_dev.reset();
        return;
    }
    if(model_read == ID_BME280){
        m_model = Model::BMX280_MODEL_BME280;
    } else if(model_read == ID_BMP280){
        m_model = Model::BMX280_MODEL_BMP280;
    } else {
        ESP_LOGE(TAG, "unknown chip ID: 0x%02X", model_read);
        m_dev.reset();
    }

}
Bmx280::~Bmx280()
{

}
} // namespace esp_cxx