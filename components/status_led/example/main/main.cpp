#include "status_led.hpp"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define brightness 20

static const char* TAG = "status led example";

extern "C" void app_main(){
    ESP_LOGI(TAG, "hello world"); 
}