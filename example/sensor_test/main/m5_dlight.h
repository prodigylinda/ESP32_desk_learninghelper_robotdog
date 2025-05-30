/**
 * @brief A Digital Ambient Light Sensor From M5Stack
 * @copyright Copyright (c) 2022 by M5Stack[https://m5stack.com]
 *
 * @Links [Unit DLight](https://docs.m5stack.com/en/unit/dlight)
 * @Links [HAT DLight](https://docs.m5stack.com/en/hat/hat_dlight)
 * @version  V0.0.3
 * @date  2022-07-27
 */
#ifndef _M5_DLIGHT_H_
#define _M5_DLIGHT_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "i2c_bus.h"

// Device I2C address
#define DLIGHT_I2C_ADDR 0x23

// Command definitions
#define DLIGHT_POWER_DOWN                     0x00
#define DLIGHT_POWER_ON                       0x01
#define DLIGHT_RESET                          0x07
#define DLIGHT_CONTINUOUSLY_H_RESOLUTION_MODE 0x10
#define DLIGHT_CONTINUOUSLY_H_RESOLUTION_MODE2 0x11
#define DLIGHT_CONTINUOUSLY_L_RESOLUTION_MODE 0x13
#define DLIGHT_ONE_TIME_H_RESOLUTION_MODE     0x20
#define DLIGHT_ONE_TIME_H_RESOLUTION_MODE2    0x21
#define DLIGHT_ONE_TIME_L_RESOLUTION_MODE     0x23

// Function declarations
esp_err_t dlight_init(i2c_bus_handle_t i2c_bus, uint8_t addr);
esp_err_t dlight_power_on(void);
esp_err_t dlight_power_off(void);
esp_err_t dlight_power_reset(void);
esp_err_t dlight_set_mode(uint8_t mode);
esp_err_t dlight_get_lux(uint16_t *lux);

#ifdef __cplusplus
}
#endif

#endif 