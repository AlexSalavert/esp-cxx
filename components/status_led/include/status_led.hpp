#pragma once

#include <cstdint>
#include <vector>

#include "esp_err.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "driver/gpio.h"

namespace esp_cxx {

enum class color_led_t : uint8_t {
    OFF = 0,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CYAN,
    MAGENTA,
    WHITE,
    COUNT
};

class StatusLed {
public:
    virtual ~StatusLed() = default;

    // non copyable, non movable
    StatusLed(const StatusLed&)            = delete;
    StatusLed& operator=(const StatusLed&) = delete;
    StatusLed(StatusLed&&)                 = delete;
    StatusLed& operator=(StatusLed&&)      = delete;

    virtual esp_err_t on()  = 0;
    virtual esp_err_t off() = 0;
    virtual esp_err_t toggle()  = 0;

protected:
    StatusLed() = default;
    bool m_on = false;
};

class SimpleLed    : public StatusLed {
public:
    SimpleLed(gpio_num_t gpio_num, bool active_high = true);
    ~SimpleLed() override;

    esp_err_t on()      override;
    esp_err_t off()     override;
    esp_err_t toggle()  override;

private:
    gpio_num_t m_gpio;
    bool m_active_high;
};

class Ws2812Led    : public StatusLed {
public: 
    Ws2812Led(gpio_num_t gpio_num, uint16_t num_leds = 1);
    ~Ws2812Led() override;

    esp_err_t on()      override;
    esp_err_t off()     override;
    esp_err_t toggle()  override;

    esp_err_t set_all_colors(color_led_t color, uint8_t brightness);
    esp_err_t set_color(uint16_t index, color_led_t color, uint8_t brightness);

    esp_err_t refresh();

private:
    gpio_num_t m_gpio;
    uint16_t m_num_leds;
    rmt_channel_handle_t   m_chan    = nullptr;
    rmt_encoder_handle_t   m_encoder = nullptr;
    std::vector<uint8_t>   m_buffer;
    std::vector<uint8_t>   m_zeros;
};
} // namespace esp_cxx