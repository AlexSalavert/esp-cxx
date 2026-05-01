#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <atomic>

namespace esp_cxx {
struct DwinEvent{
    uint16_t addr;
    uint16_t data;
};

class Dwin {
public:
    struct Config {
        uart_port_t uart_num;
        gpio_num_t  tx_gpio;
        gpio_num_t  rx_gpio;
        int         baudrate;
        size_t      queue_size = 16;
        TickType_t  timeout    = 100;
    };
    Dwin(Config config);
    ~Dwin();

    // non copyable, non movable
    Dwin(const Dwin&)            = delete;
    Dwin& operator=(const Dwin&) = delete;
    Dwin(Dwin&&)                 = delete;
    Dwin& operator=(Dwin&&)      = delete;

    static constexpr uint16_t ADDR_PAGE      = 0x0014;
    static constexpr uint16_t ADDR_BACKLIGHT = 0x0031;

    bool      event_available() const;
    esp_err_t read_event(DwinEvent *event);

    bool is_valid() const {return m_valid;}

    esp_err_t reset();

    esp_err_t set_backligth(uint8_t brt);
    esp_err_t get_backligth(DwinEvent *out);
    esp_err_t set_page(uint16_t addr);
    esp_err_t get_page(DwinEvent *out);
    esp_err_t set_VP(uint16_t addr, int16_t value);
    esp_err_t get_VP(uint16_t addr, DwinEvent *out);
    esp_err_t set_text(uint16_t addr, const char *text, size_t size);

private:
    static void rx_task(void *arg);
    void handle_response(const uint8_t* buf, size_t len);
    esp_err_t set_send_and_wait_ack(const uint8_t* buf, size_t len);
    esp_err_t get_send_and_wait_response(uint16_t addr, const uint8_t* buf, size_t len, DwinEvent *out);
    
    uart_port_t   m_uart_num;
    TickType_t    m_timeout;
    QueueHandle_t m_event_queue    = nullptr;
    QueueHandle_t m_ack_queue      = nullptr;
    QueueHandle_t m_response_queue = nullptr;
    SemaphoreHandle_t m_mutex      = nullptr;
    TaskHandle_t  m_rx_task        = nullptr;
    bool          m_valid          = false;

    std::atomic<int32_t> m_pending_addr{-1};
};

} // namespace esp_cxx
