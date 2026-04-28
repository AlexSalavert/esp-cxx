#include "i2c.hpp"
#include "esp_log.h"

static const char *TAG = "esp_cxx::I2cDevice";

namespace esp_cxx {

I2cDevice::I2cDevice(const I2cMaster& bus, Config config)
    : m_timeout_ms(config.timeout_ms)
{
    if(!bus.is_valid()){
        ESP_LOGE(TAG, "bus is not valid");
        return;
    }

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = config.address,
        .scl_speed_hz    = config.scl_speed_hz,
        .scl_wait_us     = 0,
        .flags = {
            .disable_ack_check = 0,
        },
    };
    esp_err_t err = i2c_master_bus_add_device(bus.handle(), &dev_cfg, &m_dev_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        m_dev_handle = nullptr;
    }
}

I2cDevice::~I2cDevice()
{
    if (!m_dev_handle) return;

    esp_err_t err = i2c_master_bus_rm_device(m_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_rm_device failed: %s", esp_err_to_name(err));
    }

    m_dev_handle = nullptr;
}

esp_err_t I2cDevice::write(const uint8_t* data, size_t len)
{
    if(!m_dev_handle){
        ESP_LOGE(TAG, "device not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = i2c_master_transmit(m_dev_handle, data, len, m_timeout_ms);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "write in i2c device failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t I2cDevice::read(uint8_t* data, size_t len)
{
    if(!m_dev_handle){
        ESP_LOGE(TAG, "device not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = i2c_master_receive(m_dev_handle, data, len, m_timeout_ms);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "read in i2c device failed: %s", esp_err_to_name(err));
    }
    return err; 
}

esp_err_t I2cDevice::write_read(const uint8_t* wdata, size_t wlen, uint8_t* rdata, size_t rlen)
{
    if(!m_dev_handle){
        ESP_LOGE(TAG, "device not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = i2c_master_transmit_receive(m_dev_handle, wdata, wlen, rdata, rlen, m_timeout_ms);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "write and read in i2c device failed: %s", esp_err_to_name(err));
    }
    return err; 
}
} // namespace esp_cxx