#include <string.h>
#include "m5_dlight.h"
#include "esp_log.h"

static const char *TAG = "m5_dlight";

// Static variables
static i2c_bus_device_handle_t s_dev_handle = NULL;

// Initialize the DLight sensor
esp_err_t dlight_init(i2c_bus_handle_t i2c_bus, uint8_t addr)
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
    
    // Power on the sensor
    return dlight_power_on();
}

// Write a single byte to the sensor
static esp_err_t dlight_write_byte(uint8_t cmd)
{
    esp_err_t ret = i2c_bus_write_byte(s_dev_handle, NULL_I2C_MEM_ADDR, cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write byte: %d", ret);
    }
    return ret;
}

// Read bytes from the sensor
static esp_err_t dlight_read_bytes(uint8_t *buffer, size_t size)
{
    esp_err_t ret = i2c_bus_read_bytes(s_dev_handle, NULL_I2C_MEM_ADDR, size, buffer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read bytes: %d", ret);
    }
    return ret;
}

// Power on the sensor
esp_err_t dlight_power_on(void)
{
    return dlight_write_byte(DLIGHT_POWER_ON);
}

// Power off the sensor
esp_err_t dlight_power_off(void)
{
    return dlight_write_byte(DLIGHT_POWER_DOWN);
}

// Reset the sensor
esp_err_t dlight_power_reset(void)
{
    return dlight_write_byte(DLIGHT_RESET);
}

// Set the sensor mode
esp_err_t dlight_set_mode(uint8_t mode)
{
    return dlight_write_byte(mode);
}

// Get the light intensity in lux
esp_err_t dlight_get_lux(uint16_t *lux)
{
    if (lux == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer[2];
    esp_err_t ret = dlight_read_bytes(buffer, 2);
    if (ret != ESP_OK) {
        return ret;
    }

    *lux = (buffer[0] << 8) | buffer[1];
    *lux = (uint16_t)(*lux / 1.2f);  // Convert to lux

    return ESP_OK;
} 