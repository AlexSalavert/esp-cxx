#include "dwin.hpp"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

#define UART_PORT      (uart_port_t) CONFIG_EXAMPLE_UART_PORT
#define UART_TX_GPIO   (gpio_num_t)  CONFIG_EXAMPLE_UART_TX_GPIO
#define UART_RX_GPIO   (gpio_num_t)  CONFIG_EXAMPLE_UART_RX_GPIO
#define UART_BAUDRATE  (int)         CONFIG_EXAMPLE_UART_BAUDRATE
#define TEST_VP_ADDR   (uint16_t)    CONFIG_EXAMPLE_TEST_VP_ADDR 
#define TEST_PAGE_ADDR (uint16_t)    CONFIG_EXAMPLE_TEST_PAGE_ADDR 

static const char* TAG = "dwin_hmi_example";

extern "C" void app_main()
{
    esp_cxx::DwinEvent dwin_event = {0};
    ESP_LOGI(TAG, "INIT EXAMPLE");
    esp_cxx::Dwin hmi(UART_PORT, UART_TX_GPIO, UART_RX_GPIO, UART_BAUDRATE, 16);
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "GET PAGE");

    hmi.get_page();
    hmi.read_event(dwin_event, 1000);
    ESP_LOGI(TAG, "addr: %x | data: %i", dwin_event.addr, dwin_event.data);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "SET GET PAGE");
    hmi.set_page(TEST_PAGE_ADDR);
    vTaskDelay(pdMS_TO_TICKS(1000));
    hmi.get_page();
    hmi.read_event(dwin_event, 1000);
    ESP_LOGI(TAG, "addr: %x | data: %i", dwin_event.addr, dwin_event.data);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "SET GET VP");
    hmi.set_VP(TEST_VP_ADDR, 5);
    vTaskDelay(pdMS_TO_TICKS(1000));
    hmi.get_VP(TEST_VP_ADDR);
    hmi.read_event(dwin_event, 1000);
    ESP_LOGI(TAG, "addr: %x | data: %i", dwin_event.addr, dwin_event.data);

    ESP_LOGI(TAG, "SET GET BACKLIGHT");
    hmi.set_backligth(0x7f);
    vTaskDelay(pdMS_TO_TICKS(1000));
    hmi.get_backligth();
    hmi.read_event(dwin_event, 1000);
    ESP_LOGI(TAG, "addr: %x | data: %i", dwin_event.addr, dwin_event.data);

}