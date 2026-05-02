#include "i2c.hpp"
#include "esp_log.h"

static const char *TAG = "esp_cxx::I2cSlave";
namespace esp_cxx {

I2cSlave::I2cSlave(const Config config)
    : m_timeout_ms(config.timeout_ms)
{
    #ifndef CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
        ESP_LOGE(TAG, "CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2 is not enabled. Enable it via idf.py menuconfig.");
        return;
    #endif
    const i2c_slave_config_t slave_cfg = {
        .i2c_port = config.port,
        .sda_io_num = config.sda,
        .scl_io_num = config.scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = config.send_buf,
        .receive_buf_depth = config.receive_buf,
        .slave_addr = config.address,
        .addr_bit_len = I2C_ADDR_BIT_LEN_7,
        .intr_priority = 0,
        .flags = {
            .allow_pd = 0,
            .enable_internal_pullup = config.enable_pullup,
        },
    };

    esp_err_t err = i2c_new_slave_device(&slave_cfg, &m_slave_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_new_slave_device failed: %s", esp_err_to_name(err));
        m_slave_handle = nullptr;
    }
}

I2cSlave::~I2cSlave()
{
    if(!m_slave_handle) return;
    esp_err_t err = i2c_del_slave_device(m_slave_handle);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2c_del_slave_device failed: %s", esp_err_to_name(err));
    }
    m_slave_handle = nullptr;
}

esp_err_t I2cSlave::write(const uint8_t *data, size_t len)
{
    if (!m_slave_handle) {
        ESP_LOGE(TAG, "slave not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t write_len = 0;
    esp_err_t err = i2c_slave_write(m_slave_handle, data, len, &write_len, m_timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_slave_write failed: %s", esp_err_to_name(err));
        return err;
    }
    if (write_len != len) {
        ESP_LOGE(TAG, "i2c_slave_write: only %lu of %u bytes written (buffer full)", write_len, len);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t I2cSlave::register_callback(Callbacks callbacks)
{
    if(!m_slave_handle){
        ESP_LOGE(TAG, "slave not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    m_callbacks = callbacks;
    
    i2c_slave_event_callbacks_t cbs = {
        .on_request = s_on_request,
        .on_receive = s_on_receive,
        
    };
    return i2c_slave_register_event_callbacks(m_slave_handle, &cbs, this);
}

bool I2cSlave::s_on_receive(i2c_slave_dev_handle_t handle,
                            const i2c_slave_rx_done_event_data_t* evt,
                            void* arg)
{
    auto* self = static_cast<I2cSlave*>(arg);
    if (self->m_callbacks.on_receive) {
        self->m_callbacks.on_receive(evt->buffer, evt->length);
    }
    return false;
}

bool I2cSlave::s_on_request(i2c_slave_dev_handle_t handle,
                            const i2c_slave_request_event_data_t* evt,
                            void* arg)
{
    auto* self = static_cast<I2cSlave*>(arg);
    if (self->m_callbacks.on_request) {
        self->m_callbacks.on_request();
    }
    return false;
}

} // namespace esp_cxx