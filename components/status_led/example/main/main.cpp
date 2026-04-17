#include "status_led.hpp"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#define DELAY 1000
static const char* TAG = "status led example";

extern "C" void app_main(){
esp_err_t err = ESP_OK;
#ifdef CONFIG_EXAMPLE_LED_TYPE_SIMPLE 

    // simple LED initialization
    ESP_LOGI(TAG, "INIT SIMPLE LED EXAMPLE");
    esp_cxx::SimpleLed simple_led((gpio_num_t)CONFIG_EXAMPLE_LED_GPIO, CONFIG_EXAMPLE_ACTIVE_HIGH);

    // simple LED on off
    ESP_LOGI(TAG, "on off example");
    simple_led.on();
    vTaskDelay(pdMS_TO_TICKS(DELAY));
    simple_led.off();
    vTaskDelay(pdMS_TO_TICKS(DELAY));

    // simple LED toggle example
    ESP_LOGI(TAG, "toggle example");
    for(uint8_t i = 0; i < 4; i++){
        err = simple_led.toggle();
        if(err != ESP_OK){
            ESP_LOGE(TAG, "toggle example fail");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    simple_led.off();
    ESP_LOGI(TAG, "FIN SIMPLE LED EXAMPLE");
#endif
#ifdef CONFIG_EXAMPLE_LED_TYPE_WS2812

    // ws2812 LED initialization
    ESP_LOGI(TAG, "INIT WS2812 LED EXAMPLE");
    esp_cxx::Ws2812Led ws2812_led((gpio_num_t)CONFIG_EXAMPLE_LED_GPIO, CONFIG_EXAMPLE_NUMBER_OF_LEDS);
    ws2812_led.on();

    //ws2812 LED set color example
    ESP_LOGI(TAG, "set color example");
    for (uint8_t i = static_cast<uint8_t>(esp_cxx::color_led_t::OFF);
     i < static_cast<uint8_t>(esp_cxx::color_led_t::COUNT);
     ++i)
     {
        ws2812_led.set_color(0, (esp_cxx::color_led_t)i, CONFIG_EXAMPLE_BRIGHTNESS);
        err = ws2812_led.refresh();
        if(err != ESP_OK){
            ESP_LOGE(TAG, "set color example fail");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    //ws2812 LED set all colors example
    ESP_LOGI(TAG, "set all colors example");
    for (uint8_t i = static_cast<uint8_t>(esp_cxx::color_led_t::OFF);
     i < static_cast<uint8_t>(esp_cxx::color_led_t::COUNT);
     ++i)
     {
        ws2812_led.set_all_colors((esp_cxx::color_led_t)i, CONFIG_EXAMPLE_BRIGHTNESS);
        err = ws2812_led.refresh();
        if(err != ESP_OK){
            ESP_LOGE(TAG, "set all colors fail");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    //ws2812 LED toggle example
    ESP_LOGI(TAG, "toggle example");
    for(uint8_t i = 0; i < 4; i++){
        err = ws2812_led.toggle();
        if(err != ESP_OK){
            ESP_LOGE(TAG, "toggle example fail");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    ws2812_led.off();
    ESP_LOGI(TAG, "FIN WS2812 LED EXAMPLE");

#endif
}