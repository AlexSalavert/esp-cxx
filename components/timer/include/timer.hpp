
#pragma once

#include <functional>
#include <cstdint>
#include "esp_timer.h"
#include "esp_err.h"

namespace esp_cxx {

class Timer {
public:
    using Callback = std::function<void()>;
    virtual ~Timer();

    // Non-copyable, non-movable
    Timer(const Timer&)            = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&)                 = delete;
    Timer& operator=(Timer&&)      = delete;

    virtual esp_err_t start() = 0;
    esp_err_t stop();
    bool is_running() const;
    bool is_valid() const;

protected:
    Timer(Callback callback, const char* name = "esp_cxx_timer");

    static void dispatch(void* arg);

    esp_timer_handle_t m_handle   = nullptr;
    Callback           m_callback;
    bool               m_running  = false;
};


class PeriodicTimer : public Timer {
public:
    PeriodicTimer(uint64_t period_us, Callback callback);
    esp_err_t start() override;

private:
    uint64_t m_period_us;
};

class OneShotTimer : public Timer {
public:
    OneShotTimer(uint64_t timeout_us, Callback callback);
    esp_err_t start() override;
    
private:
    uint64_t m_timeout_us;
};

} // namespace esp_cxx
