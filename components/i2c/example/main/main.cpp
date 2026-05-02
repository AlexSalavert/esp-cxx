
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
    #define I2C_SLAVE_DATA (uint8_t) CONFIG_EXAMPLE_I2C_SLAVE_DATA
#endif

static const char* TAG = "i2c example";

extern "C" void app_main()
{
    ESP_LOGI(TAG, "INIT EXAMPLE");

    // MASTER EXAMPLE
    #if(I2C_MASTER)
        ESP_LOGI(TAG, "INIT I2C MASTER EXAMPLE");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_cxx::I2cMaster::Config master_bus_config = {
            .port       = I2C_PORT,
            .sda        = I2C_SDA_GPIO,
            .scl        = I2C_SCL_GPIO,
            .timeout_ms = I2C_TIMEOUT_MS,
        };
        esp_cxx::I2cMaster i2c_master_bus(master_bus_config);
        if(i2c_master_bus.is_valid()){
            ESP_LOGI(TAG, "configure i2c master bus OK");
        }else{
            ESP_LOGE(TAG, "configure i2c master bus ERROR");
            return;
        }
        if(i2c_master_bus.probe(I2C_ADDRESS) == ESP_OK){
            ESP_LOGI(TAG, "device detected in address %x", I2C_ADDRESS);
        }else{
            ESP_LOGE(TAG, "i2c device with address %x not found", I2C_ADDRESS);
            return;
        }

        esp_cxx::I2cDevice::Config device_config = {
            .address = I2C_ADDRESS,
            .timeout_ms = I2C_TIMEOUT_MS,
        };
        esp_cxx::I2cDevice i2c_device(i2c_master_bus, device_config);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // MASTER WRITE TEST
        #if(ACTIVATE_I2C_DEVICE_WRITE_TEST)
            uint8_t write_buf[] = {I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE};
            if(i2c_device.write(write_buf, sizeof(write_buf)) == ESP_OK){
                ESP_LOGI(TAG, "i2c write test OK -> register = %x | data = %x", I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE);
            }else{
                ESP_LOGE(TAG, "i2c write test ERROR -> register = %x | data = %x", I2C_MASTER_REGISTER_WRITE, I2C_MASTER_DATA_WRITE);
            }
        #endif
        vTaskDelay(pdMS_TO_TICKS(1000));

        // MASTER READ TEST
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
        ESP_LOGI(TAG, "INIT I2C SLAVE TEST");
        esp_cxx::I2cSlave::Config slave_config = {
            .port       = I2C_PORT,
            .sda        = I2C_SDA_GPIO,
            .scl        = I2C_SCL_GPIO,
            .address    = I2C_ADDRESS,
            .timeout_ms = I2C_TIMEOUT_MS,
        };
        esp_cxx::I2cSlave i2c_slave(slave_config);

        if(i2c_slave.is_valid()){
            ESP_LOGI(TAG, "configure i2c slave bus OK");
        }else{
            ESP_LOGE(TAG, "configure i2c slave bus ERROR");
            return;
        }

        TaskHandle_t main_task = xTaskGetCurrentTaskHandle();

        esp_cxx::I2cSlave::Callbacks callbacks = {
            .on_request = [main_task](){
                // ISR context: notify main task to reload the TX buffer
                BaseType_t woken = pdFALSE;
                xTaskNotifyFromISR(main_task, 0, eNoAction, &woken);
                portYIELD_FROM_ISR(woken);
            },
            .on_receive = [](const uint8_t* data, size_t len){
                // ISR context: log must use DRAM-safe variant
                // data[0] is the register the master is accessing
                if(len > 0){
                    ESP_DRAM_LOGI(TAG, "master accessed register = 0x%02X", data[0]);
                }
                for(size_t i = 1; i < len; i++){
                    ESP_DRAM_LOGI(TAG, "  data[%u] = 0x%02X", i, data[i]);
                }
            },
        };

        if(i2c_slave.register_callback(callbacks) == ESP_OK){
            ESP_LOGI(TAG, "register callback OK");
        }else{
            ESP_LOGE(TAG, "register callback ERROR");
            return;
        }

        uint8_t response = I2C_SLAVE_DATA;
        i2c_slave.write(&response, sizeof(response));
        ESP_LOGI(TAG, "i2c slave ready at address 0x%02X, response = 0x%02X", I2C_ADDRESS, response);

        while(true){
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            i2c_slave.write(&response, sizeof(response));
            ESP_LOGI(TAG, "master read request served: 0x%02X", response);
        }

    #endif

}