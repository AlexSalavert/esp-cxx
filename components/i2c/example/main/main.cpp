
#include "i2c.hpp"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "sdkconfig.h"

#define I2C_PORT       (i2c_port_num_t) CONFIG_EXAMPLE_I2C_PORT
#define I2C_SDA_GPIO   (gpio_num_t)     CONFIG_EXAMPLE_I2C_SDA_GPIO
#define I2C_SCL_GPIO   (gpio_num_t)     CONFIG_EXAMPLE_I2C_SCL_GPIO
#define I2C_ADDRESS    (uint16_t)       CONFIG_EXAMPLE_I2C_ADDRESS
#define I2C_TIMEOUT_MS (int)            CONFIG_EXAMPLE_I2C_TIMEOUT_MS

#ifdef CONFIG_EXAMPLE_I2C_MASTER
    #define I2C_MASTER true
    #define I2C_SLAVE  false
    #ifdef CONFIG_EXAMPLE_ACTIVATE_I2C_DEVICE_WRITE_TEST
        #define ACTIVATE_I2C_DEVICE_WRITE_TEST true
        #define I2C_MASTER_REGISTER_WRITE (uint8_t) CONFIG_EXAMPLE_I2C_MASTER_REGISTER_WRITE
        #define I2C_MASTER_DATA_WRITE     (uint8_t) CONFIG_EXAMPLE_I2C_MASTER_DATA_WRITE
    #else
        #define ACTIVATE_I2C_DEVICE_WRITE_TEST false
    #endif

    #ifdef CONFIG_EXAMPLE_ACTIVATE_I2C_DEVICE_READ_TEST
        #define ACTIVATE_I2C_DEVICE_READ_TEST true
        #define I2C_MASTER_REGISTER_READ (uint8_t) CONFIG_EXAMPLE_I2C_MASTER_REGISTER_READ
    #else
        #define ACTIVATE_I2C_DEVICE_READ_TEST false
    #endif
#endif
    
#ifdef CONFIG_EXAMPLE_I2C_SLAVE
    #define I2C_MASTER false
    #define I2C_SLAVE  true
#endif

static const char* TAG = "i2c example";

extern "C" void app_main()
{
    ESP_LOGI(TAG, "INIT EXAMPLE");

    #if(I2C_MASTER)
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_cxx::I2cMaster::Config master_config = {
            .port       = I2C_PORT,
            .sda        = I2C_SDA_GPIO,
            .scl        = I2C_SCL_GPIO,
            .timeout_ms = I2C_TIMEOUT_MS,
        };
        esp_cxx::I2cMaster i2c_master(master_config);
        if(i2c_master.is_valid()){
            ESP_LOGI(TAG, "configure i2c OK");
        }else{
            ESP_LOGE(TAG, "Failure to configure i2c communication");
            return;
        }
        if(i2c_master.probe(I2C_ADDRESS) == ESP_OK){
            ESP_LOGI(TAG, "device detected in address %x", I2C_ADDRESS);
        }else{
            ESP_LOGE(TAG, "i2c device with address %x not found", I2C_ADDRESS);
            return;
        }

        esp_cxx::I2cDevice::Config device_config = {
            .address = I2C_ADDRESS,
            .timeout_ms = I2C_TIMEOUT_MS,
        };
        esp_cxx::I2cDevice i2c_device(i2c_master, device_config);
        vTaskDelay(pdMS_TO_TICKS(1000));
        #if(ACTIVATE_I2C_DEVICE_WRITE_TEST)
            uint8_t write_buf[] = {I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE};
            if(i2c_device.write(write_buf, sizeof(write_buf)) == ESP_OK){
                ESP_LOGI(TAG, "i2c write test OK -> register = %x | data = %x", I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE);
            }else{
                ESP_LOGE(TAG, "i2c write test ERROR -> register = %x | data = %x", I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE);
            }
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));
        #if(ACTIVATE_I2C_DEVICE_READ_TEST)
            uint8_t read_reg = I2C_MASTER_REGISTER_READ;
            if(i2c_device.write(&read_reg, sizeof(read_reg)) == ESP_OK){
                ESP_LOGI(TAG, "i2c write register in read test OK -> register = %x", read_reg);
            }else{
                ESP_LOGE(TAG, "i2c write register in read test ERROR -> register %x", read_reg);
            }
            uint8_t read_buf = 0;
            if(i2c_device.read(&read_buf, sizeof(read_buf)) == ESP_OK){
                ESP_LOGI(TAG, "i2c read test OK -> data = %x", read_buf);
            }else{
                ESP_LOGE(TAG, "i2c read test ERROR");
            }
            uint8_t write_read_buf = 0;
            if(i2c_device.write_read(&read_reg, sizeof(read_reg), &write_read_buf, sizeof(write_read_buf)) == ESP_OK){
                ESP_LOGI(TAG, "i2c write and read test OK -> register = %x | data = %x", read_reg, write_read_buf);
            }else{
                ESP_LOGE(TAG, "i2c write and read test ERROR -> register = %x", read_reg);
            }
        #endif

    #elif(I2C_SLAVE)
            ESP_LOGW(TAG, "The example for I2C slave is not yet developed");
    #endif

}