/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "app_https.h"
#include "app_wifi.h"
#include "servo_dog_ctrl.h"
void app_main(void)
{
    servo_dog_ctrl_config_t config = {
        .fl_gpio_num = 21,
        .fr_gpio_num = 19,
        .bl_gpio_num = 20,
        .br_gpio_num = 18,
    };
    servo_dog_ctrl_init(&config);
    app_wifi_init();
    app_https_init();
}
