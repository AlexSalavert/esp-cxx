#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer.hpp"

static const char* TAG = "timer_example";

extern "C" void app_main()
{
    int tick_count = 0;

    // ── Ejemplo 1: lambda con captura ────────────────────────────────────────
    esp_cxx::PeriodicTimer timer_a(1'000'000, [&tick_count]() {
        tick_count++;
        ESP_LOGI(TAG, "[timer_a] tick #%d", tick_count);
    });

    // ── Ejemplo 2: lambda sin captura ────────────────────────────────────────
    esp_cxx::PeriodicTimer timer_b(500'000, []() {
        ESP_LOGI(TAG, "[timer_b] fast tick");
    });

    // ── Ejemplo 3: función libre ──────────────────────────────────────────────
    auto on_slow_tick = []() {
        ESP_LOGI(TAG, "[timer_c] slow tick");
    };
    esp_cxx::PeriodicTimer timer_c(2'000'000, on_slow_tick);

    // ── Iniciar ───────────────────────────────────────────────────────────────
    timer_a.start();
    timer_b.start();
    timer_c.start();

    vTaskDelay(pdMS_TO_TICKS(5000));

    // ── Parar manualmente ─────────────────────────────────────────────────────
    timer_a.stop();
    timer_b.stop();
    timer_c.stop();

    ESP_LOGI(TAG, "done — tick_count: %d", tick_count);
}