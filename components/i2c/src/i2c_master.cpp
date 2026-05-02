#include "i2c.hpp"
#include "esp_log.h"

static const char *TAG = "esp_cxx::I2cMaster";
namespace esp_cxx {
I2cMaster::I2cMaster(Config config)
    : m_timeout_ms(config.timeout_ms)
    {
        i2c_master_bus_config_t bus_cfg = {
            .i2c_port = config.port,
            .sda_io_num = config.sda,
            .scl_io_num = config.scl,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = config.glitch_ignore_cnt,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = config.enable_pullup,
                .allow_pd = 0,
            },
        };

        esp_err_t err = i2c_new_master_bus(&bus_cfg, &m_bus_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
            m_bus_handle = nullptr;
        }
    }

I2cMaster::~I2cMaster()
{
    if(!m_bus_handle) return;
    esp_err_t err = i2c_del_master_bus(m_bus_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_del_master_bus failed: %s", esp_err_to_name(err));
    }
    m_bus_handle = nullptr;
}

esp_err_t I2cMaster::reset()
{
    if (!m_bus_handle) {
        ESP_LOGE(TAG, "bus not initialized");
        return ESP_ERR_INVALID_STATE;
    } 
    return i2c_master_bus_reset(m_bus_handle);
}

esp_err_t I2cMaster::probe(uint16_t address) const
{
    if (!m_bus_handle) {
        ESP_LOGE(TAG, "bus not initialized");
        return ESP_ERR_INVALID_STATE;
    } 
    return i2c_master_probe(m_bus_handle, address, m_timeout_ms);
}
} // namespace esp_cxx