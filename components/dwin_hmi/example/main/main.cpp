#include "dwin.hpp"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

#define UART_NUM       (uart_port_t) CONFIG_EXAMPLE_UART_NUM
#define UART_TX_GPIO   (gpio_num_t)  CONFIG_EXAMPLE_UART_TX_GPIO
#define UART_RX_GPIO   (gpio_num_t)  CONFIG_EXAMPLE_UART_RX_GPIO
#define UART_BAUDRATE  (int)         CONFIG_EXAMPLE_UART_BAUDRATE

#ifdef CONFIG_EXAMPLE_ACTIVATE_PAGE_TEST
    #define ACTIVATE_PAGE_TEST true
    #define TEST_PAGE_ADDR (uint16_t) CONFIG_EXAMPLE_TEST_PAGE_ADDR 
#else
    #define ACTIVATE_PAGE_TEST false
#endif

#ifdef CONFIG_EXAMPLE_ACTIVATE_VP_TEST
    #define ACTIVATE_VP_TEST true
    #define TEST_VP_ADDR  (uint16_t) CONFIG_EXAMPLE_TEST_VP_ADDR
    #define TEST_VP_VALUE (uint16_t) CONFIG_EXAMPLE_TEST_VP_VALUE 
#else
    #define ACTIVATE_VP_TEST false
#endif

#ifdef CONFIG_EXAMPLE_ACTIVATE_TEXT_TEST
    #define ACTIVATE_TEXT_TEST true
    #define TEST_TEXT_ADDR  (uint16_t)     CONFIG_EXAMPLE_TEST_TEXT_ADDR
    #define TEST_TEXT_VALUE (const char *) CONFIG_EXAMPLE_TEST_TEXT_VALUE
#else
    #define ACTIVATE_TEXT_TEST false
#endif

#ifdef CONFIG_EXAMPLE_ACTIVATE_BACKLIGTH_TEST
    #define ACTIVATE_BACKLIGTH_TEST true
    #define TEST_BACKLIGTH_VALUE (uint8_t) CONFIG_EXAMPLE_TEST_BACKLIGTH_VALUE
#else
    #define ACTIVATE_BACKLIGTH_TEST false
#endif

#ifdef CONFIG_EXAMPLE_ACTIVATE_EVENTS_TEST
    #define ACTIVATE_EVENTS true
#else
    #define ACTIVATE_EVENTS false
#endif

static const char* TAG = "dwin_hmi_example";
#if(ACTIVATE_EVENTS)

void receive_events(void* arg)
{
    esp_cxx::Dwin* hmi = static_cast<esp_cxx::Dwin*>(arg);
    esp_cxx::DwinEvent dwin_event = {};
    while(true){
        vTaskDelay(pdMS_TO_TICKS(100));
        if(!hmi->event_available()) continue;
        if(hmi->read_event(&dwin_event) == ESP_OK) {
            if(dwin_event.addr == esp_cxx::Dwin::ADDR_PAGE)
                ESP_LOGI(TAG, "page changed -> data: %i", dwin_event.data);
            else if(dwin_event.addr == esp_cxx::Dwin::ADDR_BACKLIGHT)
                ESP_LOGI(TAG, "backlight changed -> data: %i", dwin_event.data);
            else
                ESP_LOGI(TAG, "event recived = addr: %x | data: %i", dwin_event.addr, dwin_event.data);
        }
    }
}

#endif
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // wait for the display initialize

    ESP_LOGI(TAG, "INIT EXAMPLE");
    esp_cxx::DwinEvent dwin_event = {};
    esp_cxx::Dwin::Config hmi_config = {
        .uart_num   = UART_NUM,
        .tx_gpio    = UART_TX_GPIO,
        .rx_gpio    = UART_RX_GPIO,
        .baudrate   = UART_BAUDRATE,
        .queue_size = 16,
        .timeout    = 100,
    };
    

    #if(ACTIVATE_EVENTS)
        static esp_cxx::Dwin hmi(hmi_config);
        ESP_LOGI(TAG, "events activated");
        xTaskCreate(receive_events, "receive events", 4096, &hmi, 6, NULL);
    #else
        esp_cxx::Dwin hmi(hmi_config);
    #endif

    #if(ACTIVATE_PAGE_TEST)
        // SET PAGE
        if(hmi.set_page(TEST_PAGE_ADDR) == ESP_OK)
            ESP_LOGI(TAG, "set page (%x) OK", TEST_PAGE_ADDR);
        else
            ESP_LOGE(TAG, "set page (%x) ERROR", TEST_PAGE_ADDR);

        // GET PAGE
        if(hmi.get_page(&dwin_event) == ESP_OK)
            ESP_LOGI(TAG, "get page OK -> addr: %x | data: %i", dwin_event.addr, dwin_event.data);
        else
            ESP_LOGE(TAG, "get page ERROR");
    #endif

    #if(ACTIVATE_VP_TEST)
        // SET VP
        if(hmi.set_VP(TEST_VP_ADDR, TEST_VP_VALUE) == ESP_OK)
            ESP_LOGI(TAG, "set vp with addr = %x and value = %i OK", TEST_VP_ADDR, TEST_VP_VALUE);
        else    
            ESP_LOGE(TAG, "set vp with addr = %x and value = %i ERROR", TEST_VP_ADDR, TEST_VP_VALUE);

        // GET VP
        if(hmi.get_VP(TEST_VP_ADDR, &dwin_event) == ESP_OK)
            ESP_LOGI(TAG, "get vp OK ->  addr: %x | data: %i", dwin_event.addr, dwin_event.data);
        else    
            ESP_LOGE(TAG, "get vp with addr = %x ERROR", TEST_VP_ADDR);
    #endif

    #if(ACTIVATE_TEXT_TEST)
        // SET TEXT
        if(hmi.set_text(TEST_TEXT_ADDR,TEST_TEXT_VALUE, strlen(TEST_TEXT_VALUE)) == ESP_OK)
            ESP_LOGI(TAG, "set text with addr = %x and text = %s OK", TEST_TEXT_ADDR, TEST_TEXT_VALUE);
        else
            ESP_LOGE(TAG, "set text with addr = %x and text = %s ERROR", TEST_TEXT_ADDR, TEST_TEXT_VALUE);
    #endif

    #if(ACTIVATE_BACKLIGTH_TEST)
        // SET BACKLIGTH
        if(hmi.set_backligth(TEST_BACKLIGTH_VALUE) == ESP_OK)
            ESP_LOGI(TAG, "set backligth OK");
        else    
            ESP_LOGE(TAG, "set backligth ERROR");

        // GET BACKLIGTH
        if(hmi.get_backligth(&dwin_event) == ESP_OK)
            ESP_LOGI(TAG, "get backligth OK -> addr: %x | data: %i", dwin_event.addr, dwin_event.data);
        else
            ESP_LOGE(TAG, "get backligth ERROR");
    #endif
}