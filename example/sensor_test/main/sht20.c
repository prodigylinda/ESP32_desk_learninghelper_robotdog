#include <string.h>
#include "sht20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht20";

// Static variables
static i2c_bus_device_handle_t s_dev_handle = NULL;

// Initialize the SHT20 sensor
esp_err_t sht20_init(i2c_bus_handle_t i2c_bus, uint8_t addr)
{
    if (i2c_bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create I2C device
    s_dev_handle = i2c_bus_device_create(i2c_bus, addr, 0);
    if (s_dev_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C device");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// Check CRC
static uint8_t sht20_check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
    uint32_t remainder = (uint32_t)message_from_sensor << 8;
    remainder |= check_value_from_sensor;
    uint32_t divsor = (uint32_t)SHT20_SHIFTED_DIVISOR;
    
    for (int i = 0; i < 16; i++) {
        if (remainder & (uint32_t)1 << (23 - i)) {
            remainder ^= divsor;
        }
        divsor >>= 1;
    }
    
    return (uint8_t)remainder;
}

// Read value from sensor
static uint16_t sht20_read_value(uint8_t cmd)
{
    uint8_t buffer[3];
    esp_err_t ret;
    int counter = 0;
    
    // Write command
    ret = i2c_bus_write_byte(s_dev_handle, NULL_I2C_MEM_ADDR, cmd);
    if (ret != ESP_OK) {
        return SHT20_ERROR_I2C_TIMEOUT;
    }
    
    // Wait for measurement to complete
    while (counter < SHT20_MAX_COUNTER) {
        vTaskDelay(pdMS_TO_TICKS(SHT20_DELAY_INTERVAL));
        
        // Try to read data
        ret = i2c_bus_read_bytes(s_dev_handle, NULL_I2C_MEM_ADDR, 3, buffer);
        if (ret == ESP_OK) {
            uint16_t raw_value = ((uint16_t)buffer[0] << 8) | buffer[1];
            
            // Check CRC
            if (sht20_check_crc(raw_value, buffer[2]) == 0) {
                return raw_value & 0xFFFC;
            }
            return SHT20_ERROR_BAD_CRC;
        }
        counter++;
    }
    
    return SHT20_ERROR_I2C_TIMEOUT;
}

// Read humidity
float sht20_read_humidity(void)
{
    uint16_t raw_humidity = sht20_read_value(SHT20_TRIGGER_HUMD_MEASURE_NOHOLD);
    if (raw_humidity == SHT20_ERROR_I2C_TIMEOUT || raw_humidity == SHT20_ERROR_BAD_CRC) {
        return (float)raw_humidity;
    }
    
    float temp_rh = raw_humidity * (125.0f / 65536.0f);
    return temp_rh - 6.0f;
}

// Read temperature
float sht20_read_temperature(void)
{
    uint16_t raw_temperature = sht20_read_value(SHT20_TRIGGER_TEMP_MEASURE_NOHOLD);
    if (raw_temperature == SHT20_ERROR_I2C_TIMEOUT || raw_temperature == SHT20_ERROR_BAD_CRC) {
        return (float)raw_temperature;
    }
    
    float temp_temperature = raw_temperature * (175.72f / 65536.0f);
    return temp_temperature - 46.85f;
}

// Set resolution
esp_err_t sht20_set_resolution(uint8_t resolution)
{
    uint8_t user_register = sht20_read_user_register();
    user_register &= 0x7E;  // Clear resolution bits
    resolution &= 0x81;     // Keep only resolution bits
    user_register |= resolution;
    return sht20_write_user_register(user_register);
}

// Read user register
uint8_t sht20_read_user_register(void)
{
    uint8_t user_register;
    esp_err_t ret = i2c_bus_read_byte(s_dev_handle, SHT20_READ_USER_REG, &user_register);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read user register");
        return 0;
    }
    return user_register;
}

// Write user register
esp_err_t sht20_write_user_register(uint8_t val)
{
    return i2c_bus_write_byte(s_dev_handle, SHT20_WRITE_USER_REG, val);
}

// Check sensor status
esp_err_t sht20_check_status(void)
{
    uint8_t reg = sht20_read_user_register();
    
    ESP_LOGI(TAG, "End of battery: %s", (reg & SHT20_USER_REGISTER_END_OF_BATTERY) ? "yes" : "no");
    ESP_LOGI(TAG, "Heater enabled: %s", (reg & SHT20_USER_REGISTER_HEATER_ENABLED) ? "yes" : "no");
    ESP_LOGI(TAG, "Disable OTP reload: %s", (reg & SHT20_USER_REGISTER_DISABLE_OTP_RELOAD) ? "yes" : "no");
    
    return ESP_OK;
} 