#include "timer.hpp"
#include "esp_log.h"

static const char* TAG = "esp_cxx::OneShotTimer";

namespace esp_cxx {

// CONSTRUCTOR

OneShotTimer::OneShotTimer(uint64_t timeout_us, Callback callback)
    : Timer([this, cb = std::move(callback)]() {
        m_running = false;
        if (cb) cb();
    }, "one_shot_timer"), m_timeout_us(timeout_us)
{
    if (timeout_us == 0) {
        ESP_LOGE(TAG, "timeout_us must be > 0");
    }
}

// PUBLIC API

esp_err_t OneShotTimer::start()
{
    if(!m_handle){
        ESP_LOGE(TAG, "timer is not valid");
        return ESP_ERR_INVALID_STATE;
    }

    if (m_running) {
        ESP_LOGD(TAG, "timer already running");
        return ESP_OK;
    }

    esp_err_t err = esp_timer_start_once(m_handle, m_timeout_us);
    if (err == ESP_OK) {
        m_running = true;
        ESP_LOGD(TAG, "timer started: period=%llu us", m_timeout_us);
    } else {
        ESP_LOGE(TAG, "start failed: %s", esp_err_to_name(err));
    }
    return err;
}
} // namespace esp_cxx