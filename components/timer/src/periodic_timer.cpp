#include "timer.hpp"
#include "esp_log.h"

static const char* TAG = "esp_cxx::PeriodicTimer";

namespace esp_cxx {

// CONSTRUCTOR

PeriodicTimer::PeriodicTimer(uint64_t period_us, Callback callback)
    : Timer(std::move(callback), "periodic_timer"), m_period_us(period_us)
{
    if (period_us == 0) {
        ESP_LOGE(TAG, "period_us must be > 0");
    }
}


// PUBLIC API

esp_err_t PeriodicTimer::start()
{
    if (!m_handle) {
        ESP_LOGE(TAG, "timer is not valid");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_running) {
        ESP_LOGD(TAG, "timer already running");
        return ESP_OK;
    }

    esp_err_t err = esp_timer_start_periodic(m_handle, m_period_us);
    if (err == ESP_OK) {
        m_running = true;
        ESP_LOGD(TAG, "timer started: period=%llu us", m_period_us);
    } else {
        ESP_LOGE(TAG, "start failed: %s", esp_err_to_name(err));
    }
    return err;
}

} // namespace esp_cxx