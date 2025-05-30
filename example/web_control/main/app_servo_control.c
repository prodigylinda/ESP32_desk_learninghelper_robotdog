/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "servo_dog_ctrl.h"

static const char *TAG = "servo_control";

static int s_fl, s_fr, s_bl, s_br = 0;

esp_err_t servo_control_init(void)
{
    servo_dog_set_leg_offset(s_fl, s_bl, s_fr, s_br);
    return ESP_OK;
}

esp_err_t servo_control_get_save_value(int *fl, int *fr, int *bl, int *br)
{
    *fl = s_fl;
    *fr = s_fr;
    *bl = s_bl;
    *br = s_br;
    return ESP_OK;
}

esp_err_t servo_control_set_save_value(int fl, int fr, int bl, int br)
{
    s_fl = fl;
    s_fr = fr;
    s_bl = bl;
    s_br = br;
    servo_dog_set_leg_offset(fl, bl, fr, br);
    return ESP_OK;
}
