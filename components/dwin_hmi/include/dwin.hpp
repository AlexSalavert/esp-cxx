#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace esp_cxx {

typedef struct{
    uint16_t addr;
    uint16_t data;
} DwinEvent;

class Dwin {
public:
    Dwin(uart_port_t uart_num, gpio_num_t tx_gpio, gpio_num_t rx_gpio, 
        int baudrate, size_t event_queue_size = 16);
    ~Dwin();

    // non copyable, non movable
    Dwin(const Dwin&)            = delete;
    Dwin& operator=(const Dwin&) = delete;
    Dwin(Dwin&&)                 = delete;
    Dwin& operator=(Dwin&&)      = delete;

    bool      event_available() const;
    esp_err_t read_event(DwinEvent& event, TickType_t timeout = 0);

    bool is_valid() const {return m_valid;}

    esp_err_t reset();

    esp_err_t set_backligth(uint8_t brt);
    esp_err_t get_backligth();
    esp_err_t set_page(uint16_t addr);
    esp_err_t get_page();
    esp_err_t set_VP(uint16_t addr, int16_t value);
    esp_err_t get_VP(uint16_t addr);
    esp_err_t set_text(uint16_t addr, const char *text, size_t size);

private:
    static void rx_task(void *arg);
    void handle_response(const uint8_t* buf, size_t len);
    
    uart_port_t   m_uart_num;
    QueueHandle_t m_event_queue = nullptr;
    TaskHandle_t  m_rx_task     = nullptr;
    bool          m_valid       = false;
};

} // namespace esp_cxx
