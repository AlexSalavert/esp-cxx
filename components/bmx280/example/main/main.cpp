#include "bmx280.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

static const char* TAG = "bmx280_example";

extern "C" void app_main()
{
    ESP_LOGI(TAG, "HELLO WORLD");
}
