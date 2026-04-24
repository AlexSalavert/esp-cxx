#include "dwin.hpp"
#include "esp_log.h"
#include <string.h>
#include <string>

#define HEAD1 0x5A
#define HEAD2 0xA5
#define WRITE 0x82
#define READ  0x83

#define MACRO_SET_BACKLIGHT(ACTION, BRIGHTNESS) \
    {HEAD1, HEAD2, 0x05, ACTION, 0x00, 0x82,\
    (uint8_t)((BRIGHTNESS) >> 8), 0x00}

#define MACRO_SET_PAGE(ACTION, ADDR)                        \
    {HEAD1, HEAD2, 0x07, ACTION, 0x00, 0x84, 0x5A, 0x01,\
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR)}

#define MACRO_SET_VP(ACTION, ADDR, VP)          \
    {HEAD1, HEAD2, 0x05, ACTION,                \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR),\
    (uint8_t)((VP) >> 8), (uint8_t)(VP)}  \

#define MACRO_SET_TEXT(ACTION, ADDR, SIZE)\
    {HEAD1, HEAD2,                  \
    (uint8_t)(SIZE), ACTION,        \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR)}


static const char* TAG = "esp_cxx::dwin";

namespace esp_cxx {

Dwin::Dwin(uart_port_t uart_num, gpio_num_t tx_gpio, gpio_num_t rx_gpio, int baudrate, size_t event_queue_size)
    :m_uart_num(uart_num)
{
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return;
    }
    err = uart_set_pin(uart_num, tx_gpio, rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return;
    }
    err = uart_driver_install(uart_num, 2048, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return;
    }
    m_event_queue = xQueueCreate(event_queue_size, sizeof(DwinEvent));
    if (!m_event_queue) {
        ESP_LOGE(TAG, "xQueueCreate failed");
        uart_driver_delete(uart_num);
        return;
    }
    BaseType_t ret = xTaskCreate(rx_task, "dwin_rx", 4096, this, 5, &m_rx_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        vQueueDelete(m_event_queue);
        m_event_queue = nullptr;
        uart_driver_delete(uart_num);
        return;
    }
    m_valid = true;
}

Dwin::~Dwin()
{
    if (m_rx_task) {
        vTaskDelete(m_rx_task);
        m_rx_task = nullptr;
    }
    if (m_event_queue) {
        vQueueDelete(m_event_queue);
        m_event_queue = nullptr;
    }
    uart_driver_delete(m_uart_num);
}

void Dwin::rx_task(void* arg)
{
    Dwin* self = static_cast<Dwin*>(arg);
    uint8_t buf[128];

    while (true) {
        int len = uart_read_bytes(self->m_uart_num, buf, sizeof(buf), portMAX_DELAY);
        if (len >= 7) {
            self->handle_response(buf, len);
        }
    }
}

void Dwin::handle_response(const uint8_t* buf, size_t len)
{
    if (buf[0] != HEAD1 || buf[1] != HEAD2) {
        ESP_LOGW(TAG, "invalid frame header");
        return;
    }

    if (buf[3] != READ) return;

    if (len < 7) return;

    DwinEvent ev{};
    ev.addr     = (static_cast<uint16_t>(buf[4]) << 8) | buf[5];
    ev.data_len = buf[6] * 2;

    if (ev.data_len > sizeof(ev.data)) {
        ESP_LOGW(TAG, "data too long (%u bytes), truncating", ev.data_len);
        ev.data_len = sizeof(ev.data);
    }

    memcpy(ev.data, buf + 7, ev.data_len);

    if (xQueueSend(m_event_queue, &ev, 0) != pdTRUE) {
        ESP_LOGW(TAG, "event queue full, dropping event for addr 0x%04X", ev.addr);
    }
}

bool Dwin::event_available() const
{
    if (!m_event_queue) return false;
    return uxQueueMessagesWaiting(m_event_queue) > 0;
}

esp_err_t Dwin::read_event(DwinEvent& event, TickType_t timeout)
{
    if (!m_event_queue) return ESP_ERR_INVALID_STATE;

    if (xQueueReceive(m_event_queue, &event, timeout) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t Dwin::set_backligth(uint8_t brt)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t backlight[8] = MACRO_SET_BACKLIGHT(WRITE, brt);
    uart_write_bytes(m_uart_num, backlight, sizeof(backlight));
    return ESP_OK;
}

esp_err_t Dwin::set_page(uint16_t addr)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t page[10] = MACRO_SET_PAGE(WRITE, addr);
    uart_write_bytes(m_uart_num, page, sizeof(page));
    return ESP_OK;
}

esp_err_t Dwin::set_VP(uint16_t addr, int16_t value)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t vp[8] = MACRO_SET_VP(WRITE, addr, value);
    uart_write_bytes(m_uart_num, vp, sizeof(vp));
    return ESP_OK;
}

esp_err_t Dwin::set_text(uint16_t addr, const char *text, size_t size)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    std::string text_to_send(text, size);
    const uint8_t prefix_text[6] = MACRO_SET_TEXT(WRITE, addr, size + 3);
    uart_write_bytes(m_uart_num, prefix_text, sizeof(prefix_text));
    uart_write_bytes(m_uart_num, text_to_send.c_str(), text_to_send.size() + 1);
    return ESP_OK;
}

} // namespace esp_cxx

