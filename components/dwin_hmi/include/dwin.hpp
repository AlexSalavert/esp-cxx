#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"

namespace esp_cxx {

class Dwin {
public:
    Dwin(uart_port_t uart_num, gpio_num_t tx_gpio, gpio_num_t rx_gpio, int baudrate);
    ~Dwin();

    // non copyable, non movable
    Dwin(const Dwin&)            = delete;
    Dwin& operator=(const Dwin&) = delete;
    Dwin(Dwin&&)                 = delete;
    Dwin& operator=(Dwin&&)      = delete;

    esp_err_t set_backligth(uint8_t brt);
    esp_err_t set_page(uint16_t addr);
    esp_err_t set_VP(uint16_t addr, int16_t value);
    esp_err_t set_text(uint16_t addr, const char *text, size_t size);

    bool is_valid() const {return m_valid;}

private:
    uart_port_t m_uart_num;
    bool m_valid = false;
};

} // namespace esp_cxx