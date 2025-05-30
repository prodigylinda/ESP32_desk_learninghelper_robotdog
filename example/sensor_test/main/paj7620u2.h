/*!
 * @file paj7620u2.h
 * @brief PAJ7620U2 gesture recognition sensor driver
 * @copyright Copyright (c) 2024
 * @version V1.0
 */

#ifndef __PAJ7620U2_H
#define __PAJ7620U2_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define PAJ7620_OK              0      // No error
#define PAJ7620_ERR_BUS        -1      // I2C bus error
#define PAJ7620_ERR_VERSION    -2      // Chip version mismatch

// Device address
#define PAJ7620_I2C_ADDR       0x73
#define PAJ7620_PART_ID        0x7620

// Register definitions
#define PAJ7620_REG_BANK_SEL   0xEF    // Bank select register
#define PAJ7620_REG_PART_ID_L  0x00    // Part ID low byte
#define PAJ7620_REG_PART_ID_H  0x01    // Part ID high byte
#define PAJ7620_REG_GES_FLAG_0 0x43    // Gesture flag register 0
#define PAJ7620_REG_GES_FLAG_1 0x44    // Gesture flag register 1

// Bank definitions
#define PAJ7620_BANK_0         0
#define PAJ7620_BANK_1         1

// Gesture definitions
typedef enum {
    PAJ7620_GESTURE_NONE = 0x00,           // No gesture detected
    PAJ7620_GESTURE_RIGHT = 0x01 << 0,     // Right gesture
    PAJ7620_GESTURE_LEFT = 0x01 << 1,      // Left gesture
    PAJ7620_GESTURE_UP = 0x01 << 2,        // Up gesture
    PAJ7620_GESTURE_DOWN = 0x01 << 3,      // Down gesture
    PAJ7620_GESTURE_FORWARD = 0x01 << 4,   // Forward gesture
    PAJ7620_GESTURE_BACKWARD = 0x01 << 5,  // Backward gesture
    PAJ7620_GESTURE_CLOCKWISE = 0x01 << 6, // Clockwise gesture
    PAJ7620_GESTURE_ANTI_CLOCKWISE = 0x01 << 7, // Anti-clockwise gesture
    PAJ7620_GESTURE_WAVE = 0x01 << 8,      // Wave gesture
    PAJ7620_GESTURE_WAVE_SLOWLY_DISORDER = 0x01 << 9, // Wave slowly disorder
    PAJ7620_GESTURE_WAVE_SLOWLY_LEFT_RIGHT = (0x01 << 0) | (0x01 << 1), // Wave slowly left-right
    PAJ7620_GESTURE_WAVE_SLOWLY_UP_DOWN = (0x01 << 2) | (0x01 << 3),    // Wave slowly up-down
    PAJ7620_GESTURE_WAVE_SLOWLY_FORWARD_BACKWARD = (0x01 << 4) | (0x01 << 5), // Wave slowly forward-backward
} paj7620_gesture_t;

// Gesture description structure
typedef struct {
    paj7620_gesture_t gesture;
    const char *description;
} paj7620_gesture_desc_t;

// Device structure
typedef struct {
    i2c_bus_handle_t i2c_dev;  // I2C bus handle
    bool high_rate;            // High rate mode flag
} paj7620_dev_t;

/**
 * @brief Initialize PAJ7620U2 device
 * @param dev Device handle
 * @param i2c_bus I2C bus handle
 * @return 0 on success, negative error code on failure
 */
int paj7620_init(paj7620_dev_t *dev, i2c_bus_handle_t i2c_bus);

/**
 * @brief Deinitialize PAJ7620U2 device
 * @param dev Device handle
 * @return 0 on success, negative error code on failure
 */
int paj7620_deinit(paj7620_dev_t *dev);

/**
 * @brief Set gesture recognition rate mode
 * @param dev Device handle
 * @param high_rate true for high rate mode, false for normal rate mode
 */
void paj7620_set_high_rate(paj7620_dev_t *dev, bool high_rate);

/**
 * @brief Get current gesture
 * @param dev Device handle
 * @return Detected gesture
 */
paj7620_gesture_t paj7620_get_gesture(paj7620_dev_t *dev);

/**
 * @brief Get gesture description
 * @param gesture Gesture to get description for
 * @return Gesture description string
 */
const char *paj7620_get_gesture_desc(paj7620_gesture_t gesture);

#ifdef __cplusplus
}
#endif

#endif /* __PAJ7620U2_H */ 