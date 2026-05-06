#include "bmx280.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

#define I2C_PORT       (i2c_port_num_t) -1
#define I2C_SDA_GPIO   (gpio_num_t)     2
#define I2C_SCL_GPIO   (gpio_num_t)     1
#define I2C_TIMEOUT_MS (int)            100
static const char* TAG = "bmx280_example";

extern "C" void app_main()
{
    ESP_LOGI(TAG, "HELLO WORLD");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_cxx::I2cMaster::Config bus_config = {
        .port       = I2C_PORT,
        .sda        = I2C_SDA_GPIO,
        .scl        = I2C_SCL_GPIO,
        .timeout_ms = I2C_TIMEOUT_MS,
    };
    esp_cxx::I2cMaster i2c_bus(bus_config);
    esp_cxx::Bmx280 sensor(i2c_bus, esp_cxx::BMX280_I2C_ADDR_LOW);

    esp_cxx::Bmx280::Config config;
    sensor.get_config(config);
    ESP_LOGI(TAG, "osrs_t = 0x%x | osrs_p = 0x%x | osrs_h = 0x%x | filter = 0x%x | mode = 0x%x | stanby = 0x%x", 
        static_cast<uint8_t>(config.osrs_t),
        static_cast<uint8_t>(config.osrs_p),
        static_cast<uint8_t>(config.osrs_h),
        static_cast<uint8_t>(config.filter),
        static_cast<uint8_t>(config.mode),
        static_cast<uint8_t>(config.standby));
    sensor.set_config(esp_cxx::Bmx280::Config::highPrecision());

    
    sensor.get_config(config);
    ESP_LOGI(TAG, "osrs_t = 0x%x | osrs_p = 0x%x | osrs_h = 0x%x | filter = 0x%x | mode = 0x%x | stanby = 0x%x", 
        static_cast<uint8_t>(config.osrs_t),
        static_cast<uint8_t>(config.osrs_p),
        static_cast<uint8_t>(config.osrs_h),
        static_cast<uint8_t>(config.filter),
        static_cast<uint8_t>(config.mode),
        static_cast<uint8_t>(config.standby));
    if(sensor.is_valid()){
        ESP_LOGI(TAG, "sensor create OK");
    }else{
        ESP_LOGE(TAG, "sensor create ERROR");
    }
    while(true){
        vTaskDelay(pdMS_TO_TICKS(1000));
        float temp = 0.0;
        sensor.read_temperature(temp);
        float press = 0.0;
        sensor.read_pressure(press);
        float hum = 0.0;
        sensor.read_humidity(hum);
        float alt = 0.0;
        sensor.read_altitude(alt);
        ESP_LOGI(TAG, "temp = %.2f °C | press = %.2f hPA | HUM = %.2f %% | ALT = %.2f m", temp, press / 100.0f, hum, alt);
        esp_cxx::Bmx280::Data data;
        sensor.read(data);
        ESP_LOGI(TAG, "temp = %.2f °C | press = %.2f hPA | HUM = %.2f %%", data.temp, data.press / 100.0f, data.hum);

    }
}
