/**
 * Copyright (C) 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bme68x.h"
#include "i2c_bus.h"
#include "bme_esp_interface.h"
#include "esp_rom_sys.h"

static const char *TAG = "BME688_ESP";

/* Static variables */
static i2c_bus_device_handle_t i2c_device_handle = NULL;

/* Function implementations */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    esp_err_t ret;

    (void)intf_ptr;
    ret = i2c_bus_read_bytes(i2c_device_handle, reg_addr, len, reg_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %d", ret);
        return BME68X_E_COM_FAIL;
    }

    return BME68X_OK;
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    esp_err_t ret;

    (void)intf_ptr;

    ret = i2c_bus_write_bytes(i2c_device_handle, reg_addr, len, reg_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %d", ret);
        return BME68X_E_COM_FAIL;
    }

    return BME68X_OK;
}

void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    esp_rom_delay_us(period);

}

int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf, i2c_bus_handle_t i2c_bus_handle, uint8_t i2c_addr)
{
    int8_t rslt = BME68X_OK;

    if (bme != NULL && i2c_bus_handle != NULL) {
        /* Create I2C device */
        i2c_device_handle = i2c_bus_device_create(i2c_bus_handle, i2c_addr, 0);
        if (i2c_device_handle == NULL) {
            ESP_LOGE(TAG, "I2C device create failed");
            return BME68X_E_COM_FAIL;
        }

        /* Set interface functions */
        bme->read = bme68x_i2c_read;
        bme->write = bme68x_i2c_write;
        bme->intf = BME68X_I2C_INTF;
        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &i2c_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    } else {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

void bme68x_interface_deinit(i2c_bus_device_handle_t *dev_handle)
{
    if (i2c_device_handle) {
        i2c_bus_device_delete(&i2c_device_handle);
        if (dev_handle) {
            *dev_handle = NULL;
        }
    }
}
