#include "status_led.hpp"


namespace esp_cxx {

SimpleLed::SimpleLed(gpio_num_t gpio_num, bool active_high)
    : m_gpio(gpio_num), m_active_high(active_high)
{
    gpio_reset_pin(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    off();
}

SimpleLed::~SimpleLed()
{
    off();
    gpio_reset_pin(m_gpio);
}

esp_err_t SimpleLed::on()
{
    const bool level = m_active_high ? true : false;
    esp_err_t err = gpio_set_level(m_gpio, level);
    if(err == ESP_OK) m_on = true;
    return err;
}

esp_err_t SimpleLed::off()
{
    const bool level = m_active_high ? false : true;
    esp_err_t err = gpio_set_level(m_gpio, level);
    if(err == ESP_OK) m_on = false;
    return err;
}

esp_err_t SimpleLed::toggle()
{
    return m_on ? off() : on();
}

} // namespace esp_cxx