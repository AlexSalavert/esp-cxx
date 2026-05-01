#include "dwin.hpp"
#include "esp_log.h"
#include <string.h>
#include <vector>

#define HEAD1 0x5A
#define HEAD2 0xA5
#define WRITE 0x82
#define READ  0x83

#define MACRO_RESET {HEAD1, HEAD2, 0x07, WRITE, 0x00, 0x04, 0x55, 0xAA, HEAD1, HEAD2}

#define MACRO_SET_PAGE(ADDR)                            \
    {HEAD1, HEAD2, 0x07, WRITE, 0x00, 0x84, 0x5A, 0x01, \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR)}

#define MACRO_SET_VP(ADDR, VALUE)              \
    {HEAD1, HEAD2, 0x05, WRITE,                \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR),   \
    (uint8_t)((VALUE) >> 8), (uint8_t)(VALUE)} \

#define MACRO_GET_VP(ADDR)                   \
    {HEAD1, HEAD2, 0x04, READ,               \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR), 0x01}     

#define MACRO_SET_BACKLIGHT(BRIGHTNESS)    \
    {HEAD1, HEAD2, 0x05, WRITE, 0x00, 0x82,\
    (uint8_t)(BRIGHTNESS), 0x00}

#define MACRO_SET_TEXT(ADDR, SIZE)         \
    {HEAD1, HEAD2, (uint8_t)(SIZE), WRITE, \
    (uint8_t)((ADDR) >> 8), (uint8_t)(ADDR)}


static const char* TAG = "esp_cxx::dwin";

namespace esp_cxx {

Dwin::Dwin(Config config)
    :m_uart_num(config.uart_num), m_timeout(config.timeout)
{
    uart_config_t uart_config = {
        .baud_rate = config.baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = 0,
    };
    esp_err_t err = uart_param_config(config.uart_num, &uart_config);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
        return;
    }
    err = uart_set_pin(config.uart_num, config.tx_gpio, config.rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
        return;
    }
    err = uart_driver_install(config.uart_num, 2048, 2048, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
        return;
    }
    m_event_queue = xQueueCreate(config.queue_size, sizeof(DwinEvent));
    if (!m_event_queue) {
        ESP_LOGE(TAG, "xQueueCreate failed");
        uart_driver_delete(config.uart_num);
        return;
    }
    m_ack_queue = xQueueCreate(config.queue_size, sizeof(uint8_t[6]));
    if(!m_ack_queue){
        ESP_LOGE(TAG, "xQueueCreate failed");
        uart_driver_delete(config.uart_num);
        vQueueDelete(m_event_queue);
        m_event_queue = nullptr;
        return;
    }

    m_response_queue = xQueueCreate(config.queue_size, sizeof(DwinEvent));
    if(!m_response_queue){
        ESP_LOGE(TAG, "xQueueCreate failed");
        uart_driver_delete(config.uart_num);
        vQueueDelete(m_event_queue);
        m_event_queue = nullptr;
        vQueueDelete(m_ack_queue);
        m_ack_queue = nullptr;
        return;
    }

    m_mutex = xSemaphoreCreateMutex();
    if(!m_mutex){
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed");
        uart_driver_delete(config.uart_num);
        vQueueDelete(m_event_queue);
        vQueueDelete(m_ack_queue);
        vQueueDelete(m_response_queue);
        m_event_queue = nullptr;
        m_ack_queue = nullptr;
        m_response_queue = nullptr;
        return;
    }

    BaseType_t ret = xTaskCreate(rx_task, "dwin_rx", 4096, this, 5, &m_rx_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        vQueueDelete(m_event_queue);
        vQueueDelete(m_ack_queue);
        vQueueDelete(m_response_queue);
        vSemaphoreDelete(m_mutex);
        m_event_queue = nullptr;
        m_ack_queue = nullptr;
        m_response_queue = nullptr;
        m_mutex = nullptr;
        uart_driver_delete(config.uart_num);
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
    if (m_ack_queue) {
        vQueueDelete(m_ack_queue);
        m_ack_queue = nullptr;
    }
    if (m_response_queue) {
        vQueueDelete(m_response_queue);
        m_response_queue = nullptr;
    }
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }
    uart_driver_delete(m_uart_num);
}

void Dwin::rx_task(void* arg)
{
    Dwin* self = static_cast<Dwin*>(arg);
    uint8_t buf[128];
    while (true) {
        int len = uart_read_bytes(self->m_uart_num, buf, sizeof(buf), pdMS_TO_TICKS(20));
        if(len >= 9){
            self->handle_response(buf, len);
        } else if(len == 6){
            uint8_t check[6];
            memcpy(check, buf, sizeof(check));
            if (xQueueSend(self->m_ack_queue, check, 0) != pdTRUE) {
                ESP_LOGW(TAG, "event queue full");
            }
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

    DwinEvent ev{};
    ev.addr = (static_cast<uint16_t>(buf[4]) << 8) | buf[5];

    ev.data = (static_cast<uint16_t>(buf[7]) << 8) | buf[8];

    if(ev.addr == m_pending_addr){
        if(xQueueSend(m_response_queue, &ev, 0) != pdTRUE){
            ESP_LOGW(TAG, "event queue full, dropping event for addr 0x%04X", ev.addr);
        }
        return;
    } 
    if (xQueueSend(m_event_queue, &ev, 0) != pdTRUE) {
        ESP_LOGW(TAG, "event queue full, dropping event for addr 0x%04X", ev.addr);
    }
}

bool Dwin::event_available() const
{
    if (!m_event_queue) return false;
    return uxQueueMessagesWaiting(m_event_queue) > 0;
}

esp_err_t Dwin::read_event(DwinEvent *event)
{
    if (!m_event_queue) return ESP_ERR_INVALID_STATE;

    if (xQueueReceive(m_event_queue, event, m_timeout) == pdTRUE) {
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t Dwin::set_send_and_wait_ack(const uint8_t* buf, size_t len)
{
    uint8_t ack[6];
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    uart_write_bytes(m_uart_num, buf, len);
    if (xQueueReceive(m_ack_queue, ack, m_timeout) != pdTRUE) {
        xSemaphoreGive(m_mutex);
        return ESP_ERR_TIMEOUT;
    }
    xSemaphoreGive(m_mutex);
    if (ack[0] != HEAD1 || ack[1] != HEAD2) {
        ESP_LOGW(TAG, "invalid ACK frame");
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

esp_err_t Dwin::get_send_and_wait_response(uint16_t addr, const uint8_t* buf, size_t len, DwinEvent *out)
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_pending_addr.store(addr);
    uart_write_bytes(m_uart_num, buf, len);
    if(xQueueReceive(m_response_queue, out, m_timeout) != pdTRUE){
        m_pending_addr.store(-1);
        xSemaphoreGive(m_mutex);
        return ESP_ERR_TIMEOUT;
    }
    m_pending_addr.store(-1);
    xSemaphoreGive(m_mutex);
    return ESP_OK;
}

esp_err_t Dwin::reset()
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    } 
    const uint8_t reset[10] = MACRO_RESET;
    return set_send_and_wait_ack(reset, sizeof(reset));
}

esp_err_t Dwin::set_backligth(uint8_t brt)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t backlight[8] = MACRO_SET_BACKLIGHT(brt);
    return set_send_and_wait_ack(backlight, sizeof(backlight));
}

esp_err_t Dwin::get_backligth(DwinEvent *out)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t backlight[7] = MACRO_GET_VP(ADDR_BACKLIGHT);
    return get_send_and_wait_response(ADDR_BACKLIGHT, backlight, sizeof(backlight), out);
}

esp_err_t Dwin::set_page(uint16_t addr)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t page[10] = MACRO_SET_PAGE(addr);
    return set_send_and_wait_ack(page, sizeof(page));
}

esp_err_t Dwin::get_page(DwinEvent *out)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t page[7] = MACRO_GET_VP(ADDR_PAGE);
    return get_send_and_wait_response(ADDR_PAGE, page, sizeof(page), out);
}

esp_err_t Dwin::set_VP(uint16_t addr, int16_t value)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t vp[8] = MACRO_SET_VP(addr, value);
    return set_send_and_wait_ack(vp, sizeof(vp));
}

esp_err_t Dwin::get_VP(uint16_t addr, DwinEvent *out)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t vp[7] = MACRO_GET_VP(addr);
    return get_send_and_wait_response(addr, vp, sizeof(vp), out);
}

esp_err_t Dwin::set_text(uint16_t addr, const char *text, size_t size)
{
    if(!m_valid){
        ESP_LOGE(TAG, "Dwin not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    const uint8_t prefix[6] = MACRO_SET_TEXT(addr, size + 3);
    std::vector<uint8_t> frame(prefix, prefix + sizeof(prefix));
    frame.insert(frame.end(), text, text + size);
    return set_send_and_wait_ack(frame.data(), frame.size());
}

} // namespace esp_cxx

