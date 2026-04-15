#include "status_led.hpp"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>


static const char *TAG = "ws2812";

/* ---- Timing constants (10 MHz clock → 0.1 µs / tick) ---- */
static constexpr uint32_t RMT_RESOLUTION_HZ = 10'000'000;
static constexpr uint16_t T0H_TICKS = 3;   // 0.3 µs
static constexpr uint16_t T0L_TICKS = 9;   // 0.9 µs
static constexpr uint16_t T1H_TICKS = 9;   // 0.9 µs
static constexpr uint16_t T1L_TICKS = 3;   // 0.3 µs
static constexpr size_t   BYTES_PER_LED = 3; // GRB

namespace {

void color_to_rgb(esp_cxx::color_led_t color, uint8_t brightness, uint8_t &r, uint8_t &g, uint8_t &b)
{
    r = g = b = 0;
    switch (color) {
        case esp_cxx::color_led_t::RED:     r = brightness; break;
        case esp_cxx::color_led_t::GREEN:   g = brightness; break;
        case esp_cxx::color_led_t::BLUE:    b = brightness; break;
        case esp_cxx::color_led_t::YELLOW:  r = brightness; g = brightness; break;
        case esp_cxx::color_led_t::CYAN:    g = brightness; b = brightness; break;
        case esp_cxx::color_led_t::MAGENTA: r = brightness; b = brightness; break;
        case esp_cxx::color_led_t::WHITE:   r = g = b = brightness;  break;
        case esp_cxx::color_led_t::OFF:
        default:                break;
    }
}

} // anonymous namespace

namespace esp_cxx {

Ws2812Led::Ws2812Led(gpio_num_t gpio_num, uint16_t num_leds)
    : m_gpio(gpio_num), m_num_leds(num_leds)
{
    m_buffer.resize(static_cast<size_t>(num_leds) * BYTES_PER_LED, 0);
    m_zeros.resize(static_cast<size_t>(m_num_leds) * BYTES_PER_LED, 0);

    /* TX channel */
    rmt_tx_channel_config_t tx_cfg{};
    tx_cfg.gpio_num          = gpio_num;
    tx_cfg.clk_src           = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz     = RMT_RESOLUTION_HZ;
    tx_cfg.mem_block_symbols = 64;
    tx_cfg.trans_queue_depth = 4;

    esp_err_t ret = rmt_new_tx_channel(&tx_cfg, &m_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_tx_channel failed: %s", esp_err_to_name(ret));
        return;
    }

    /* Bytes encoder with WS2812 bit-timing */
    rmt_bytes_encoder_config_t enc_cfg{};
    enc_cfg.bit0.duration0      = T0H_TICKS;
    enc_cfg.bit0.level0         = 1;
    enc_cfg.bit0.duration1      = T0L_TICKS;
    enc_cfg.bit0.level1         = 0;
    enc_cfg.bit1.duration0      = T1H_TICKS;
    enc_cfg.bit1.level0         = 1;
    enc_cfg.bit1.duration1      = T1L_TICKS;
    enc_cfg.bit1.level1         = 0;
    enc_cfg.flags.msb_first     = 1;

    ret = rmt_new_bytes_encoder(&enc_cfg, &m_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_new_bytes_encoder failed: %s", esp_err_to_name(ret));
        rmt_del_channel(m_chan);
        return;
    }

    ret = rmt_enable(m_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "rmt_enable failed: %s", esp_err_to_name(ret));
        rmt_del_encoder(m_encoder);
        rmt_del_channel(m_chan);
        return;
    }

    ESP_LOGI(TAG, "init OK — GPIO %d, %u LED(s)", gpio_num, num_leds);
}

Ws2812Led::~Ws2812Led()
{
    rmt_disable(m_chan);
    rmt_del_encoder(m_encoder);
    rmt_del_channel(m_chan);
}

esp_err_t Ws2812Led::on()
{
    if(m_on) return ESP_OK;
    m_on = true;
    esp_err_t err = refresh();
    if(err != ESP_OK) m_on = false;
    return err;
}

esp_err_t Ws2812Led::off()
{
    if(!m_on) return ESP_OK;
    m_on = false;
    esp_err_t err = refresh();
    if(err != ESP_OK) m_on = true;
    return err;
}

esp_err_t Ws2812Led::toggle()
{
    m_on = !m_on;
    esp_err_t err = refresh();
    if(err != ESP_OK) m_on = !m_on;
    return err;
}

esp_err_t Ws2812Led::set_all_colors(color_led_t color, uint8_t brightness)
{
    esp_err_t err = ESP_OK;
    for (uint16_t i = 0; i < m_num_leds; ++i) {
        err = set_color(i, color, brightness);
        if(err != ESP_OK) break;
    }
    return err;
}
esp_err_t Ws2812Led::set_color(uint16_t index, color_led_t color, uint8_t brightness)
{
    if (index >= m_num_leds) {
        ESP_LOGE(TAG, "LED index %u out of bounds (max %u)", index, m_num_leds - 1);
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t r, g, b;
    color_to_rgb(color, brightness, r, g, b);

    m_buffer[index * 3 + 0] = g;
    m_buffer[index * 3 + 1] = r;
    m_buffer[index * 3 + 2] = b;
    
    return ESP_OK;
}

esp_err_t Ws2812Led::refresh()
{
    if (!m_chan || !m_encoder) return ESP_ERR_INVALID_STATE;

    rmt_transmit_config_t tx_cfg{};
    tx_cfg.loop_count = 0;

    const std::vector<uint8_t>* data;

    if(m_on){
        data = &m_buffer;
    }else{
        data = &m_zeros;
    }
    esp_err_t err = rmt_transmit(m_chan, m_encoder,
                                 data->data(),
                                 data->size(), &tx_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_transmit failed: %s", esp_err_to_name(err));
        return err;
    }
    err = rmt_tx_wait_all_done(m_chan, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "rmt_tx_wait_all_done failed: %s", esp_err_to_name(err));
    }
    esp_rom_delay_us(50); 
    return err;
}

} // namespace esp_cxx