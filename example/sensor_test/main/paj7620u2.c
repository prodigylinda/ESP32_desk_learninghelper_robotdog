/*!
 * @file paj7620u2.c
 * @brief PAJ7620U2 gesture recognition sensor driver implementation
 * @copyright Copyright (c) 2024
 * @version V1.0
 */

#include <string.h>
#include "paj7620u2.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "PAJ7620U2";

// Gesture descriptions
static const paj7620_gesture_desc_t gesture_descriptions[] = {
    {PAJ7620_GESTURE_NONE, "None"},
    {PAJ7620_GESTURE_RIGHT, "Right"},
    {PAJ7620_GESTURE_LEFT, "Left"},
    {PAJ7620_GESTURE_UP, "Up"},
    {PAJ7620_GESTURE_DOWN, "Down"},
    {PAJ7620_GESTURE_FORWARD, "Forward"},
    {PAJ7620_GESTURE_BACKWARD, "Backward"},
    {PAJ7620_GESTURE_CLOCKWISE, "Clockwise"},
    {PAJ7620_GESTURE_ANTI_CLOCKWISE, "Anti-Clockwise"},
    {PAJ7620_GESTURE_WAVE, "Wave"},
    {PAJ7620_GESTURE_WAVE_SLOWLY_DISORDER, "WaveSlowlyDisorder"},
    {PAJ7620_GESTURE_WAVE_SLOWLY_LEFT_RIGHT, "WaveSlowlyLeftRight"},
    {PAJ7620_GESTURE_WAVE_SLOWLY_UP_DOWN, "WaveSlowlyUpDown"},
    {PAJ7620_GESTURE_WAVE_SLOWLY_FORWARD_BACKWARD, "WaveSlowlyForwardBackward"}
};

// Initialization register array
static const uint8_t init_register_array[][2] = {
    {0xEF, 0x00},
    {0x32, 0x29},
    {0x33, 0x01},
    {0x34, 0x00},
    {0x35, 0x01},
    {0x36, 0x00},
    {0x37, 0x07},
    {0x38, 0x17},
    {0x39, 0x06},
    {0x3A, 0x12},
    {0x3F, 0x00},
    {0x40, 0x02},
    {0x41, 0xFF},
    {0x42, 0x01},
    {0x46, 0x2D},
    {0x47, 0x0F},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x4A, 0x1E},
    {0x4B, 0x00},
    {0x4C, 0x20},
    {0x4D, 0x00},
    {0x4E, 0x1A},
    {0x4F, 0x14},
    {0x50, 0x00},
    {0x51, 0x10},
    {0x52, 0x00},
    {0x5C, 0x02},
    {0x5D, 0x00},
    {0x5E, 0x10},
    {0x5F, 0x3F},
    {0x60, 0x27},
    {0x61, 0x28},
    {0x62, 0x00},
    {0x63, 0x03},
    {0x64, 0xF7},
    {0x65, 0x03},
    {0x66, 0xD9},
    {0x67, 0x03},
    {0x68, 0x01},
    {0x69, 0xC8},
    {0x6A, 0x40},
    {0x6D, 0x04},
    {0x6E, 0x00},
    {0x6F, 0x00},
    {0x70, 0x80},
    {0x71, 0x00},
    {0x72, 0x00},
    {0x73, 0x00},
    {0x74, 0xF0},
    {0x75, 0x00},
    {0x80, 0x42},
    {0x81, 0x44},
    {0x82, 0x04},
    {0x83, 0x20},
    {0x84, 0x20},
    {0x85, 0x00},
    {0x86, 0x10},
    {0x87, 0x00},
    {0x88, 0x05},
    {0x89, 0x18},
    {0x8A, 0x10},
    {0x8B, 0x01},
    {0x8C, 0x37},
    {0x8D, 0x00},
    {0x8E, 0xF0},
    {0x8F, 0x81},
    {0x90, 0x06},
    {0x91, 0x06},
    {0x92, 0x1E},
    {0x93, 0x0D},
    {0x94, 0x0A},
    {0x95, 0x0A},
    {0x96, 0x0C},
    {0x97, 0x05},
    {0x98, 0x0A},
    {0x99, 0x41},
    {0x9A, 0x14},
    {0x9B, 0x0A},
    {0x9C, 0x3F},
    {0x9D, 0x33},
    {0x9E, 0xAE},
    {0x9F, 0xF9},
    {0xA0, 0x48},
    {0xA1, 0x13},
    {0xA2, 0x10},
    {0xA3, 0x08},
    {0xA4, 0x30},
    {0xA5, 0x19},
    {0xA6, 0x10},
    {0xA7, 0x08},
    {0xA8, 0x24},
    {0xA9, 0x04},
    {0xAA, 0x1E},
    {0xAB, 0x1E},
    {0xCC, 0x19},
    {0xCD, 0x0B},
    {0xCE, 0x13},
    {0xCF, 0x64},
    {0xD0, 0x21},
    {0xD1, 0x0F},
    {0xD2, 0x88},
    {0xE0, 0x01},
    {0xE1, 0x04},
    {0xE2, 0x41},
    {0xE3, 0xD6},
    {0xE4, 0x00},
    {0xE5, 0x0C},
    {0xE6, 0x0A},
    {0xE7, 0x00},
    {0xE8, 0x00},
    {0xE9, 0x00},
    {0xEE, 0x07},
    {0xEF, 0x01},
    {0x00, 0x1E},
    {0x01, 0x1E},
    {0x02, 0x0F},
    {0x03, 0x10},
    {0x04, 0x02},
    {0x05, 0x00},
    {0x06, 0xB0},
    {0x07, 0x04},
    {0x08, 0x0D},
    {0x09, 0x0E},
    {0x0A, 0x9C},
    {0x0B, 0x04},
    {0x0C, 0x05},
    {0x0D, 0x0F},
    {0x0E, 0x02},
    {0x0F, 0x12},
    {0x10, 0x02},
    {0x11, 0x02},
    {0x12, 0x00},
    {0x13, 0x01},
    {0x14, 0x05},
    {0x15, 0x07},
    {0x16, 0x05},
    {0x17, 0x07},
    {0x18, 0x01},
    {0x19, 0x04},
    {0x1A, 0x05},
    {0x1B, 0x0C},
    {0x1C, 0x2A},
    {0x1D, 0x01},
    {0x1E, 0x00},
    {0x21, 0x00},
    {0x22, 0x00},
    {0x23, 0x00},
    {0x25, 0x01},
    {0x26, 0x00},
    {0x27, 0x39},
    {0x28, 0x7F},
    {0x29, 0x08},
    {0x30, 0x03},
    {0x31, 0x00},
    {0x32, 0x1A},
    {0x33, 0x1A},
    {0x34, 0x07},
    {0x35, 0x07},
    {0x36, 0x01},
    {0x37, 0xFF},
    {0x38, 0x36},
    {0x39, 0x07},
    {0x3A, 0x00},
    {0x3E, 0xFF},
    {0x3F, 0x00},
    {0x40, 0x77},
    {0x41, 0x40},
    {0x42, 0x00},
    {0x43, 0x30},
    {0x44, 0xA0},
    {0x45, 0x5C},
    {0x46, 0x00},
    {0x47, 0x00},
    {0x48, 0x58},
    {0x4A, 0x1E},
    {0x4B, 0x1E},
    {0x4C, 0x00},
    {0x4D, 0x00},
    {0x4E, 0xA0},
    {0x4F, 0x80},
    {0x50, 0x00},
    {0x51, 0x00},
    {0x52, 0x00},
    {0x53, 0x00},
    {0x54, 0x00},
    {0x57, 0x80},
    {0x59, 0x10},
    {0x5A, 0x08},
    {0x5B, 0x94},
    {0x5C, 0xE8},
    {0x5D, 0x08},
    {0x5E, 0x3D},
    {0x5F, 0x99},
    {0x60, 0x45},
    {0x61, 0x40},
    {0x63, 0x2D},
    {0x64, 0x02},
    {0x65, 0x96},
    {0x66, 0x00},
    {0x67, 0x97},
    {0x68, 0x01},
    {0x69, 0xCD},
    {0x6A, 0x01},
    {0x6B, 0xB0},
    {0x6C, 0x04},
    {0x6D, 0x2C},
    {0x6E, 0x01},
    {0x6F, 0x32},
    {0x71, 0x00},
    {0x72, 0x01},
    {0x73, 0x35},
    {0x74, 0x00},
    {0x75, 0x33},
    {0x76, 0x31},
    {0x77, 0x01},
    {0x7C, 0x84},
    {0x7D, 0x03},
    {0x7E, 0x01}
};

// Timing constants
#define GES_REACTION_TIME    50    // Gesture reaction time
#define GES_ENTRY_TIME      2000   // Entry time for forward/backward gestures
#define GES_QUIT_TIME      1000    // Quit time for gestures

// Private function declarations
static int select_bank(paj7620_dev_t *dev, uint8_t bank);
static int write_reg(paj7620_dev_t *dev, uint8_t reg, const void *data, size_t len);
static int read_reg(paj7620_dev_t *dev, uint8_t reg, void *data, size_t len);

int paj7620_init(paj7620_dev_t *dev, i2c_bus_handle_t i2c_bus)
{
    if (!dev || !i2c_bus) {
        return PAJ7620_ERR_BUS;
    }

    // Create device handle with device address
    dev->i2c_dev = i2c_bus_device_create(i2c_bus, PAJ7620_I2C_ADDR, 0);
    if (!dev->i2c_dev) {
        ESP_LOGE(TAG, "Failed to create device handle");
        return PAJ7620_ERR_BUS;
    }

    dev->high_rate = true;

    // Check device ID
    uint16_t part_id;
    if (select_bank(dev, PAJ7620_BANK_0) != PAJ7620_OK) {
        ESP_LOGE(TAG, "Failed to select bank 0");
        i2c_bus_device_delete(&dev->i2c_dev);
        return PAJ7620_ERR_BUS;
    }

    if (read_reg(dev, PAJ7620_REG_PART_ID_L, &part_id, 2) != 2) {
        ESP_LOGE(TAG, "Failed to read part ID");
        i2c_bus_device_delete(&dev->i2c_dev);
        return PAJ7620_ERR_BUS;
    }

    if (part_id != PAJ7620_PART_ID) {
        ESP_LOGE(TAG, "Invalid part ID: 0x%04X", part_id);
        i2c_bus_device_delete(&dev->i2c_dev);
        return PAJ7620_ERR_VERSION;
    }

    // Initialize registers
    for (size_t i = 0; i < sizeof(init_register_array) / sizeof(init_register_array[0]); i++) {
        if (write_reg(dev, init_register_array[i][0], &init_register_array[i][1], 1) != PAJ7620_OK) {
            ESP_LOGE(TAG, "Failed to write register 0x%02X", init_register_array[i][0]);
            i2c_bus_device_delete(&dev->i2c_dev);
            return PAJ7620_ERR_BUS;
        }
    }

    // Select bank 0
    if (select_bank(dev, PAJ7620_BANK_0) != PAJ7620_OK) {
        ESP_LOGE(TAG, "Failed to select bank 0");
        i2c_bus_device_delete(&dev->i2c_dev);
        return PAJ7620_ERR_BUS;
    }

    ESP_LOGI(TAG, "PAJ7620U2 initialized successfully");
    return PAJ7620_OK;
}

void paj7620_set_high_rate(paj7620_dev_t *dev, bool high_rate)
{
    if (dev) {
        dev->high_rate = high_rate;
    }
}

paj7620_gesture_t paj7620_get_gesture(paj7620_dev_t *dev)
{
    if (!dev) {
        return PAJ7620_GESTURE_NONE;
    }

    uint8_t gesture = 0;
    paj7620_gesture_t result = PAJ7620_GESTURE_NONE;

    // Read gesture flag register 1
    if (read_reg(dev, PAJ7620_REG_GES_FLAG_1, &gesture, 1) == 1) {
        result = (paj7620_gesture_t)(((uint16_t)gesture) << 8);
        if (result == PAJ7620_GESTURE_WAVE) {
            ESP_LOGD(TAG, "Wave gesture detected");
            vTaskDelay(pdMS_TO_TICKS(GES_QUIT_TIME));
            return result;
        }
    }

    // Read gesture flag register 0
    if (read_reg(dev, PAJ7620_REG_GES_FLAG_0, &gesture, 1) == 1) {
        result = (paj7620_gesture_t)(((uint16_t)gesture) & 0x00FF);
        
        if (!dev->high_rate) {
            uint8_t tmp;
            vTaskDelay(pdMS_TO_TICKS(GES_ENTRY_TIME));
            if (read_reg(dev, PAJ7620_REG_GES_FLAG_0, &tmp, 1) == 1) {
                result = (paj7620_gesture_t)(((uint16_t)result) | tmp);
            }
        }

        if (result != PAJ7620_GESTURE_NONE) {
            switch (result) {
                case PAJ7620_GESTURE_RIGHT:
                    ESP_LOGD(TAG, "Right gesture detected");
                    break;
                case PAJ7620_GESTURE_LEFT:
                    ESP_LOGD(TAG, "Left gesture detected");
                    break;
                case PAJ7620_GESTURE_UP:
                    ESP_LOGD(TAG, "Up gesture detected");
                    break;
                case PAJ7620_GESTURE_DOWN:
                    ESP_LOGD(TAG, "Down gesture detected");
                    break;
                case PAJ7620_GESTURE_FORWARD:
                    ESP_LOGD(TAG, "Forward gesture detected");
                    vTaskDelay(pdMS_TO_TICKS(dev->high_rate ? GES_QUIT_TIME/5 : GES_QUIT_TIME));
                    break;
                case PAJ7620_GESTURE_BACKWARD:
                    ESP_LOGD(TAG, "Backward gesture detected");
                    vTaskDelay(pdMS_TO_TICKS(dev->high_rate ? GES_QUIT_TIME/5 : GES_QUIT_TIME));
                    break;
                case PAJ7620_GESTURE_CLOCKWISE:
                    ESP_LOGD(TAG, "Clockwise gesture detected");
                    break;
                case PAJ7620_GESTURE_ANTI_CLOCKWISE:
                    ESP_LOGD(TAG, "Anti-clockwise gesture detected");
                    break;
                default:
                    uint8_t tmp;
                    if (read_reg(dev, PAJ7620_REG_GES_FLAG_1, &tmp, 1) == 1 && tmp) {
                        result = PAJ7620_GESTURE_WAVE;
                        ESP_LOGD(TAG, "Wave gesture detected");
                    } else {
                        switch (result) {
                            case PAJ7620_GESTURE_WAVE_SLOWLY_LEFT_RIGHT:
                                ESP_LOGD(TAG, "Left-right wave gesture detected");
                                break;
                            case PAJ7620_GESTURE_WAVE_SLOWLY_UP_DOWN:
                                ESP_LOGD(TAG, "Up-down wave gesture detected");
                                break;
                            case PAJ7620_GESTURE_WAVE_SLOWLY_FORWARD_BACKWARD:
                                ESP_LOGD(TAG, "Forward-backward wave gesture detected");
                                break;
                            default:
                                result = PAJ7620_GESTURE_WAVE_SLOWLY_DISORDER;
                                ESP_LOGD(TAG, "Wave disorder gesture detected");
                                break;
                        }
                    }
                    break;
            }
        }
    }

    return result;
}

const char *paj7620_get_gesture_desc(paj7620_gesture_t gesture)
{
    for (size_t i = 0; i < sizeof(gesture_descriptions) / sizeof(gesture_descriptions[0]); i++) {
        if (gesture == gesture_descriptions[i].gesture) {
            return gesture_descriptions[i].description;
        }
    }
    return "Unknown";
}

static int select_bank(paj7620_dev_t *dev, uint8_t bank)
{
    return write_reg(dev, PAJ7620_REG_BANK_SEL, &bank, 1);
}

static int write_reg(paj7620_dev_t *dev, uint8_t reg, const void *data, size_t len)
{
    if (!dev || !data) {
        return 0;
    }
    esp_err_t ret = i2c_bus_write_bytes(dev->i2c_dev, reg, len, (const uint8_t *)data);
    return ret;
}

static int read_reg(paj7620_dev_t *dev, uint8_t reg, void *data, size_t len)
{
    if (!dev || !data) {
        return 0;
    }
    esp_err_t ret = i2c_bus_read_bytes(dev->i2c_dev, reg, len, (uint8_t *)data);
    return (ret == ESP_OK) ? len : 0;
}

/**
 * @brief Deinitialize PAJ7620U2 device
 * @param dev Device handle
 * @return 0 on success, negative error code on failure
 */
int paj7620_deinit(paj7620_dev_t *dev)
{
    if (!dev || !dev->i2c_dev) {
        return PAJ7620_ERR_BUS;
    }

    esp_err_t ret = i2c_bus_device_delete(&dev->i2c_dev);
    return (ret == ESP_OK) ? PAJ7620_OK : PAJ7620_ERR_BUS;
}
