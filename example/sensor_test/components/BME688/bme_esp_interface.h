/**
 * Copyright (C) 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BME_ESP_INTERFACE_H_
#define _BME_ESP_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bme68x.h"
#include "i2c_bus.h"

/* Function declarations */
BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void bme68x_delay_us(uint32_t period, void *intf_ptr);

/**
 * @brief Initialize BME688 device interface
 * 
 * @param bme Pointer to BME68x device structure
 * @param intf Interface type (BME68X_I2C_INTF or BME68X_SPI_INTF)
 * @param i2c_bus_handle I2C bus handle, must be initialized before calling this function
 * @param i2c_addr I2C device address
 * @return int8_t BME68X_OK on success, error code on failure
 */
int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf, i2c_bus_handle_t i2c_bus_handle, uint8_t i2c_addr);

/**
 * @brief Deinitialize BME688 device interface
 * 
 * @param dev_handle Pointer to I2C device handle, will be set to NULL after deinit
 */
void bme68x_interface_deinit(i2c_bus_device_handle_t *dev_handle);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_INTERFACE_H_ */
