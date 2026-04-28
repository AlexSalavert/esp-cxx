#pragma once

#include <functional>

#include <driver/i2c_master.h>
#include <driver/i2c_slave.h>
#include "esp_err.h"
#include "driver/gpio.h"


namespace esp_cxx {
class I2cMaster{
public:
    struct Config{
        i2c_port_num_t port;
        gpio_num_t sda;
        gpio_num_t scl;
        uint8_t glitch_ignore_cnt = 7;
        bool enable_pullup = true;
        int timeout_ms;
    };
    I2cMaster(Config config);
    ~I2cMaster();

    // non copyable, non movable
    I2cMaster(const I2cMaster&)            = delete;
    I2cMaster& operator=(const I2cMaster&) = delete;
    I2cMaster(const I2cMaster&&)            = delete;
    I2cMaster& operator=(const I2cMaster&&) = delete;
    
    esp_err_t reset();
    esp_err_t probe(uint16_t address) const;
    bool is_valid() const { return m_bus_handle != nullptr; }

    i2c_master_bus_handle_t handle() const { return m_bus_handle; }

private:
    i2c_master_bus_handle_t m_bus_handle = nullptr;
    int m_timeout_ms = -1;
};

class I2cDevice{
public:
    struct Config{
        uint16_t address;
        uint32_t scl_speed_hz = 400000;
        int timeout_ms = -1;
    };
    I2cDevice(const I2cMaster& bus, Config config);
    ~I2cDevice();

    I2cDevice(const I2cDevice&)               = delete;
    I2cDevice& operator = (const I2cDevice&)  = delete;
    I2cDevice(const I2cDevice&&)              = delete;
    I2cDevice& operator = (const I2cDevice&&) = delete;

    esp_err_t write(const uint8_t* data, size_t len);
    esp_err_t read(uint8_t* data, size_t len);
    esp_err_t write_read(const uint8_t* wdata, size_t wlen, uint8_t* rdata, size_t rlen);

    bool is_valid() const { return m_dev_handle != nullptr; }
private:
    i2c_master_dev_handle_t m_dev_handle = nullptr;
    int m_timeout_ms = -1;
};

class I2cSlave{
public:
    using RequestCallback = std::function<void()>;
    using ReceiveCallback = std::function<void(const uint8_t* data, size_t len)>;

    struct Config{
        i2c_port_num_t port;
        gpio_num_t sda;
        gpio_num_t scl;
        uint16_t address;
        uint32_t send_buf    = 128;
        uint32_t receive_buf = 128;
        bool enable_pullup   = true;
        int timeout_ms          = -1;
    };

    struct Callbacks{
        RequestCallback on_request = nullptr;
        ReceiveCallback on_receive = nullptr;
    };

    I2cSlave(const Config config);
    ~I2cSlave();

    I2cSlave(const I2cSlave&)               = delete;
    I2cSlave& operator = (const I2cSlave&)  = delete;
    I2cSlave(const I2cSlave&&)              = delete;
    I2cSlave& operator = (const I2cSlave&&) = delete;

    esp_err_t register_callback(Callbacks callbacks);
    esp_err_t write(const uint8_t *data, size_t len);
    
private:

    static bool s_on_request(i2c_slave_dev_handle_t handle,
                             const i2c_slave_request_event_data_t* evt,
                             void* arg);
    static bool s_on_receive(i2c_slave_dev_handle_t handle,
                             const i2c_slave_rx_done_event_data_t* evt,
                             void* arg);
    
    i2c_slave_dev_handle_t m_slave_handle = nullptr;
    Callbacks m_callbacks;
    int m_timeout_ms = -1;
};
} // namespace esp_cxx