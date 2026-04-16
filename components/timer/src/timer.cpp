#include "timer.hpp"
#include "esp_log.h"

static const char* TAG = "esp_cxx::Timer";

namespace esp_cxx {

// CONSTRUCTOR

Timer::Timer(Callback callback, const char* name)
    : m_callback(std::move(callback))
{
    if(!m_callback){
        ESP_LOGE(TAG, "callback must not be null");
        return;
    }

    const esp_timer_create_args_t args = {
        .callback         = &Timer::dispatch,
        .arg              = this,
        .dispatch_method  = ESP_TIMER_TASK,
        .name             = name,
        .skip_unhandled_events = false,
    };

    esp_err_t err = esp_timer_create(&args, &m_handle);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(err));
        m_handle = nullptr;
    }
}

// DESTRUCTOR

Timer::~Timer()
{
    if (!m_handle) return;

    if (m_running) {
        esp_timer_stop(m_handle);
    }
    esp_timer_delete(m_handle);
    m_handle = nullptr;
}

// PUBLIC API

esp_err_t Timer::stop(){
    if (!m_handle) {
        ESP_LOGE(TAG, "timer is not valid");
        return ESP_ERR_INVALID_STATE;
    }

    if (!m_running) {
        ESP_LOGD(TAG, "timer already stopped");
        return ESP_OK;
    }

    esp_err_t err = esp_timer_stop(m_handle);
    if (err == ESP_OK) {
        m_running = false;
        ESP_LOGD(TAG, "timer stopped");
    } else {
        ESP_LOGE(TAG, "stop failed: %s", esp_err_to_name(err));
    }
    return err;
}

bool Timer::is_running() const
{
    return m_running;
}

bool Timer::is_valid() const
{
    return m_handle != nullptr;
}

// PRIVATE

// Static callback required by esp_timer C API.
// Recovers 'this' from arg and delegates to the stored std::function.
void Timer::dispatch(void* arg)
{
    auto* self = static_cast<Timer*>(arg);
    if (self && self->m_callback) {
        self->m_callback();
    }
}

} // namespace esp_cxx