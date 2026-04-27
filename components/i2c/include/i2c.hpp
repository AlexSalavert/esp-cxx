#pragma once

#include <driver/i2c_master.h>
#include <driver/i2c_slave.h>
#include "esp_err.h"
#include "driver/gpio.h"

namespace esp_cxx {
class I2cMaster{
public:
    I2cMaster(i2c_port_num_t port,
              gpio_num_t sda,
              gpio_num_t scl,
              bool enable_internal_pullup = true,
              uint8_t glitch_ignore_cnt = 7);
    ~I2cMaster();

    // non copyable, non movable
    I2cMaster(const I2cMaster&)            = delete;
    I2cMaster& operator=(const I2cMaster&) = delete;
    I2cMaster(const I2cMaster&&)            = delete;
    I2cMaster& operator=(const I2cMaster&&) = delete;

    esp_err_t probe(uint16_t address, uint32_t timeout_ms = 50) const;
    bool is_valid() const { return m_bus_handle != nullptr; }

    i2c_master_bus_handle_t handle() const { return m_bus_handle; }

private:
    i2c_master_bus_handle_t m_bus_handle = nullptr;
};
///*
class I2cDevice{
public:
    I2cDevice(const I2cMaster& bus, uint16_t address, uint32_t scl_speed_hz = 400000);
    ~I2cDevice();

    I2cDevice(const I2cDevice&)               = delete;
    I2cDevice& operator = (const I2cDevice&)  = delete;
    I2cDevice(const I2cDevice&&)              = delete;
    I2cDevice& operator = (const I2cDevice&&) = delete;

    esp_err_t write(const uint8_t* data, size_t len, int timeout_ms = -1);
    esp_err_t read(uint8_t* data, size_t len, int timeout_ms = -1);
    esp_err_t write_read(const uint8_t* wdata, size_t wlen, uint8_t* rdata, size_t rlen, int timeout_ms = -1);

    bool is_valid() const { return m_dev_handle != nullptr; }
private:
    i2c_master_dev_handle_t m_dev_handle = nullptr;
};
//*/
} // namespace esp_cxx