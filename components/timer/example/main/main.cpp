#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer.hpp"

static const char* TAG = "timer_example";

extern "C" void app_main()
{
    int tick_count = 0;

    // PeriodicTimer -> lambda with capture
    esp_cxx::PeriodicTimer periodic(1000000, [&tick_count]() {
        tick_count++;
        ESP_LOGI(TAG, "[periodic] tick: %d", tick_count);
    });

    // PeriodicTimer -> lambda without capture
    esp_cxx::PeriodicTimer fast_periodic(500000, []() {
        ESP_LOGI(TAG, "[fast_periodic] fast tick");
    });

    // OneShotTimer -> it fires only once
    esp_cxx::OneShotTimer one_shot(3000000, []() {
        ESP_LOGI(TAG, "[one_shot] fired after 3 seconds");
    });

    // START
    periodic.start();
    fast_periodic.start();
    one_shot.start();

    ESP_LOGI(TAG, "all timers started");

    vTaskDelay(pdMS_TO_TICKS(5000));

    // STOP
    periodic.stop();
    fast_periodic.stop();
    one_shot.stop();

    ESP_LOGI(TAG, "done, tick_count: %d", tick_count);
}