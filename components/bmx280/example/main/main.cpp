#include "bmx280.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

#define I2C_PORT       (i2c_port_num_t)CONFIG_EXAMPLE_I2C_PORT
#define I2C_SDA_GPIO   (gpio_num_t)CONFIG_EXAMPLE_I2C_SDA_GPIO
#define I2C_SCL_GPIO   (gpio_num_t)CONFIG_EXAMPLE_I2C_SCL_GPIO

#ifdef CONFIG_EXAMPLE_BMX280_ADDRESS_LOW
    #define BMX280_I2C_ADDRESS    (uint8_t)esp_cxx::BMX280_I2C_ADDR_LOW
#elif CONFIG_EXAMPLE_BMX280_ADDRESS_HIGH
    #define BMX280_I2C_ADDRESS    (uint8_t)esp_cxx::BMX280_I2C_ADDR_HIGH
#endif

#ifdef CONFIG_EXAMPLE_BME280
    #define HUMIDITY_ENABLE true
#elif CONFIG_EXAMPLE_BMP280
    #define HUMIDITY_ENABLE false
#endif

#ifdef CONFIG_EXAMPLE_WEATHER_MONNITORING
    #define BMX280_DEFAULT_CONFIG esp_cxx::Bmx280::Config::weatherMonitoring()
#elif CONFIG_EXAMPLE_HUMIDITY_SENSING
    #define BMX280_DEFAULT_CONFIG esp_cxx::Bmx280::Config::humiditySensing()
#elif CONFIG_EXAMPLE_INDOOR_NAVIGATION
    #define BMX280_DEFAULT_CONFIG esp_cxx::Bmx280::Config::indoorNavigation()
#elif CONFIG_EXAMPLE_GAMING
    #define BMX280_DEFAULT_CONFIG esp_cxx::Bmx280::Config::gaming()
#elif CONFIG_EXAMPLE_HIGH_PRECISSION
    #define BMX280_DEFAULT_CONFIG esp_cxx::Bmx280::Config::highPrecision()
#endif



static const char* TAG = "bmx280_example";

extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_cxx::I2cMaster::Config bus_config = {
        .port       = I2C_PORT,
        .sda        = I2C_SDA_GPIO,
        .scl        = I2C_SCL_GPIO,
        .timeout_ms = 100,
    };
    esp_cxx::I2cMaster i2c_bus(bus_config);
    esp_cxx::Bmx280 sensor(i2c_bus, BMX280_I2C_ADDRESS);

    if(sensor.is_valid()){
        ESP_LOGI(TAG, "sensor create OK");
    }else{
        ESP_LOGE(TAG, "sensor create ERROR");
    }

    sensor.set_config(BMX280_DEFAULT_CONFIG);

    esp_cxx::Bmx280::Config config;
    sensor.get_config(config);
    ESP_LOGI(TAG, "osrs_t = 0x%x | osrs_p = 0x%x | osrs_h = 0x%x | filter = 0x%x | mode = 0x%x | stanby = 0x%x", 
        static_cast<uint8_t>(config.osrs_t),
        static_cast<uint8_t>(config.osrs_p),
        static_cast<uint8_t>(config.osrs_h),
        static_cast<uint8_t>(config.filter),
        static_cast<uint8_t>(config.mode),
        static_cast<uint8_t>(config.standby));

    esp_cxx::Bmx280::Mode mode;

    sensor.get_mode(mode);
    ESP_LOGI(TAG, "MODE = 0x%x", mode);

    while(true){
        float temp = 0.0, press = 0.0, hum = 0.0, alt = 0.0;

        sensor.read_temperature(temp);
        ESP_LOGI(TAG,"Temperature = %.2f °C", temp);
        sensor.read_pressure(press);
        ESP_LOGI(TAG,"Pressure    = %.2f hPA ", press / 100.0f);
        #if(HUMIDITY_ENABLE)
            sensor.read_humidity(hum);
            ESP_LOGI(TAG,"Humidity    = %.2f %%", hum);
        #endif
        sensor.read_altitude(alt);
        ESP_LOGI(TAG,"Altitude    = %.2f m \n", alt);

        esp_cxx::Bmx280::Data data;
        sensor.read(data);
        ESP_LOGI(TAG, "TEMP = %.2f °C | PRESS = %.2f hPA | HUM = %.2f %% | ALT = %.2f m\n", data.temp, data.press / 100.0f, data.hum, data.alt);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
