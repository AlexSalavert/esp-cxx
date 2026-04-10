#include "timer.hpp"
#include "esp_log.h"

static const char* TAG = "esp_cxx::PeriodicTimer";

namespace esp_cxx {

// ─── Constructor ────────────────────────────────────────────────────────────

PeriodicTimer::PeriodicTimer(uint64_t period_us, Callback callback)
    : m_callback(std::move(callback)), m_period_us(period_us)
{
    if (period_us == 0) {
        ESP_LOGE(TAG, "period_us must be > 0");
        return;
    }

    if (!m_callback) {
        ESP_LOGE(TAG, "callback must not be null");
        return;
    }

    const esp_timer_create_args_t args = {
        .callback         = &PeriodicTimer::dispatch,
        .arg              = this,
        .dispatch_method  = ESP_TIMER_TASK,
        .name             = "esp_cxx_timer"
    };

    esp_err_t err = esp_timer_create(&args, &m_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_timer_create failed: %s", esp_err_to_name(err));
        m_handle = nullptr;
    }
}

// ─── Destructor ──────────────────────────────────────────────────────────────

PeriodicTimer::~PeriodicTimer()
{
    if (!m_handle) return;

    if (m_running) {
        esp_timer_stop(m_handle);
    }
    esp_timer_delete(m_handle);
    m_handle = nullptr;
}

// ─── Move semantics ──────────────────────────────────────────────────────────

PeriodicTimer::PeriodicTimer(PeriodicTimer&& other) noexcept
    : m_handle   (other.m_handle),
      m_callback (std::move(other.m_callback)),
      m_period_us(other.m_period_us),
      m_running  (other.m_running)
{
    // Transfer ownership — esp_timer stores 'arg = this', so we need to
    // update the arg pointer to point to the new object.
    if (m_handle) {
        // Stop, recreate with new 'this', restore running state
        bool was_running = m_running;
        esp_timer_stop(m_handle);
        esp_timer_delete(m_handle);
        m_handle  = nullptr;
        m_running = false;

        const esp_timer_create_args_t args = {
            .callback        = &PeriodicTimer::dispatch,
            .arg             = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name            = "esp_cxx_timer"
        };

        if (esp_timer_create(&args, &m_handle) == ESP_OK && was_running) {
            esp_timer_start_periodic(m_handle, m_period_us);
            m_running = true;
        }
    }

    other.m_handle   = nullptr;
    other.m_running  = false;
    other.m_period_us = 0;
}

PeriodicTimer& PeriodicTimer::operator=(PeriodicTimer&& other) noexcept
{
    if (this != &other) {
        this->~PeriodicTimer();
        new (this) PeriodicTimer(std::move(other));
    }
    return *this;
}

// ─── Public API ──────────────────────────────────────────────────────────────

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

esp_err_t PeriodicTimer::stop()
{
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

bool PeriodicTimer::is_running() const
{
    return m_running;
}

bool PeriodicTimer::is_valid() const
{
    return m_handle != nullptr;
}

// ─── Private ─────────────────────────────────────────────────────────────────

// Static callback required by esp_timer C API.
// Recovers 'this' from arg and delegates to the stored std::function.
void PeriodicTimer::dispatch(void* arg)
{
    auto* self = static_cast<PeriodicTimer*>(arg);
    if (self && self->m_callback) {
        self->m_callback();
    }
}

} // namespace esp_cxx