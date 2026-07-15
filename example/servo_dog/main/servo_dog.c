/*
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "servo_dog_ctrl.h"
#include "iot_button.h"
#include "button_gpio.h"

static const char *TAG = "servo_dog";

#define BUTTON_NUM 1

static button_handle_t g_btns[BUTTON_NUM] = {0};

static servo_dog_state_t s_dog_state = DOG_STATE_IDLE;

static void button_single_click_cb(void *arg, void *data)
{
    s_dog_state++;
    if (s_dog_state >= DOG_STATE_MAX) {
        s_dog_state = 0;
    }
    ESP_LOGI(TAG, "button_single_click_cb, state=%d", s_dog_state);
    servo_dog_ctrl_send(s_dog_state, NULL);
}

static void button_double_click_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "button_double_click_cb, state=%d", s_dog_state);
    servo_dog_ctrl_send(s_dog_state, NULL);
}

static void button_long_press_cb(void *arg, void *data)
{
    servo_dog_ctrl_send(DOG_STATE_INSTALLATION, NULL);
}

static void button_init(void)
{
    button_config_t cfg = {
        .long_press_time = 1500,
        .short_press_time = 300,
    };
    button_gpio_config_t gpio_cfg = {
        .active_level = 0,
        .gpio_num = 9,
    };

    iot_button_new_gpio_device(&cfg, &gpio_cfg, &g_btns[0]);
    iot_button_register_cb(g_btns[0], BUTTON_SINGLE_CLICK, NULL, button_single_click_cb, NULL);
    iot_button_register_cb(g_btns[0], BUTTON_DOUBLE_CLICK, NULL, button_double_click_cb, NULL);
    iot_button_register_cb(g_btns[0], BUTTON_LONG_PRESS_START, NULL, button_long_press_cb, NULL);
}

void app_main(void)
{
    button_init();

    servo_dog_ctrl_config_t config = {
        .fl_gpio_num = 21,
        .fr_gpio_num = 19,
        .bl_gpio_num = 20,
        .br_gpio_num = 18,
    };

    servo_dog_ctrl_init(&config);
}
