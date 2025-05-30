/**
 * @brief SHT20 Temperature and Humidity Sensor Driver
 * @copyright Copyright (c) 2022 by DFRobot
 * @version  V0.0.1
 * @date  2024-03-21
 */
#ifndef _SHT20_H_
#define _SHT20_H_

#include <stdint.h>
#include "i2c_bus.h"

// Error codes
#define SHT20_ERROR_I2C_TIMEOUT                    998
#define SHT20_ERROR_BAD_CRC                        999

// Device address and commands
#define SHT20_SLAVE_ADDRESS                        0x40
#define SHT20_TRIGGER_TEMP_MEASURE_HOLD            0xE3
#define SHT20_TRIGGER_HUMD_MEASURE_HOLD            0xE5
#define SHT20_TRIGGER_TEMP_MEASURE_NOHOLD          0xF3
#define SHT20_TRIGGER_HUMD_MEASURE_NOHOLD          0xF5
#define SHT20_WRITE_USER_REG                       0xE6
#define SHT20_READ_USER_REG                        0xE7
#define SHT20_SOFT_RESET                           0xFE

// User register masks and values
#define SHT20_USER_REGISTER_RESOLUTION_MASK        0x81
#define SHT20_USER_REGISTER_RESOLUTION_RH12_TEMP14 0x00
#define SHT20_USER_REGISTER_RESOLUTION_RH8_TEMP12  0x01
#define SHT20_USER_REGISTER_RESOLUTION_RH10_TEMP13 0x80
#define SHT20_USER_REGISTER_RESOLUTION_RH11_TEMP11 0x81
#define SHT20_USER_REGISTER_END_OF_BATTERY         0x40
#define SHT20_USER_REGISTER_HEATER_ENABLED         0x04
#define SHT20_USER_REGISTER_DISABLE_OTP_RELOAD     0x02

// Timing parameters
#define SHT20_MAX_WAIT                             100
#define SHT20_DELAY_INTERVAL                       10
#define SHT20_SHIFTED_DIVISOR                      0x988000
#define SHT20_MAX_COUNTER                          (SHT20_MAX_WAIT / SHT20_DELAY_INTERVAL)

// Function declarations
esp_err_t sht20_init(i2c_bus_handle_t i2c_bus, uint8_t addr);
esp_err_t sht20_set_resolution(uint8_t resolution);
esp_err_t sht20_write_user_register(uint8_t val);
uint8_t sht20_read_user_register(void);
float sht20_read_humidity(void);
float sht20_read_temperature(void);
esp_err_t sht20_check_status(void);

#endif 