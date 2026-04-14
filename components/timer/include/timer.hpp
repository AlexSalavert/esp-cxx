
#pragma once

#include <functional>
#include <cstdint>
#include "esp_timer.h"
#include "esp_err.h"

namespace esp_cxx {

/**
 * @brief Periodic timer with RAII and full C++ callback support.
 *
 * Wraps esp_timer to execute a callback at regular intervals.
 * Supports lambdas with captures, std::function, and free functions.
 *
 * Usage:
 * @code
 * esp_cxx::PeriodicTimer timer(1'000'000, []() {
 *     printf("tick\n");
 * });
 * timer.start();
 * @endcode
 */
class PeriodicTimer {
public:
    using Callback = std::function<void()>;

    /**
     * @brief Constructs and configures the timer.
     * @param period_us  Period in microseconds. Must be > 0.
     * @param callback   Function to execute on each tick.
     *                   Accepts lambdas, std::function, free functions.
     */
    PeriodicTimer(uint64_t period_us, Callback callback);

    /**
     * @brief Stops and releases the timer automatically.
     */
    ~PeriodicTimer();

    // Non-copyable — a timer is a unique resource
    PeriodicTimer(const PeriodicTimer&)            = delete;
    PeriodicTimer& operator=(const PeriodicTimer&) = delete;

    // Movable
    PeriodicTimer(PeriodicTimer&& other) noexcept;
    PeriodicTimer& operator=(PeriodicTimer&& other) noexcept;

    /**
     * @brief Starts the periodic timer.
     * @return ESP_OK on success.
     *         ESP_ERR_INVALID_STATE if the timer was not created correctly.
     *         ESP_ERR_INVALID_ARG   if period_us is 0 or callback is null.
     */
    esp_err_t start();

    /**
     * @brief Stops the timer. Does not release resources.
     * @return ESP_OK always, even if the timer was already stopped.
     */
    esp_err_t stop();

    /**
     * @brief Returns true if the timer is currently running.
     */
    bool is_running() const;

    /**
     * @brief Returns true if the timer was created successfully.
     */
    bool is_valid() const;

private:
    static void dispatch(void* arg);

    esp_timer_handle_t  m_handle    = nullptr;
    Callback            m_callback;
    uint64_t            m_period_us = 0;
    bool                m_running   = false;
};

} // namespace esp_cxx
