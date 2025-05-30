/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"

#include "esp_lv_fs.h"
#include "esp_lv_decoder.h"
#include "mmap_generate_gif.h"
#include "mmap_generate_audio.h"

#include "iot_button.h"
#include "button_gpio.h"

static const char *TAG = "main";

static mmap_assets_handle_t asset_DriverA_handle = NULL;
static mmap_assets_handle_t asset_audio          = NULL;

static esp_lv_fs_handle_t fs_DriverA_handle      = NULL;
static esp_lv_decoder_handle_t decoder_handle    = NULL;

/** 
 * @brief To set animation mode 
 */
typedef enum {
    PLAY_MODE_OFF = 1,             /** animation off */
    PLAY_MODE_ONESHOT,             /** play animation one time*/
    PLAY_MODE_CONTINUOUS,          /** play animation conitnuously */
} animation_mode_t;

/** 
 * @brief To control audio mode 
 */
typedef enum {
    AUDIO_MODE_IDLE = 1,             /** animation off */
    AUDIO_MODE_ONESHOT,             /** play animation one time*/
    AUDIO_MODE_CONTINUOUS,          /** play animation conitnuously */
} audio_mode_t;

static animation_mode_t play_mode_g     = PLAY_MODE_OFF;
static audio_mode_t     audio_mode_g    = AUDIO_MODE_IDLE;
static uint32_t max_size_A = 39;        // last frame of animation
static uint32_t animation_delay_g = 16;  // control the speed of animation

static void image_mmap_init()
{
    const mmap_assets_config_t config_DriveA = {
        .partition_label = "assets",
        .max_files = MMAP_GIF_FILES,
        .checksum = MMAP_GIF_CHECKSUM,
        .flags = {
            .app_bin_check = true,
            .mmap_enable = true,
        }
    };
    ESP_ERROR_CHECK(mmap_assets_new(&config_DriveA, &asset_DriverA_handle));

    const fs_cfg_t fs_cfg_a = {
        .fs_letter = 'A',
        .fs_assets = asset_DriverA_handle,
        .fs_nums = MMAP_GIF_FILES
    };
    esp_lv_fs_desc_init(&fs_cfg_a, &fs_DriverA_handle);

    esp_lv_decoder_init(&decoder_handle); //Initialize this after lvgl starts
}

static void audio_mmap_init()
{
    const mmap_assets_config_t config = {
        .partition_label = "audio",
        .max_files = MMAP_AUDIO_FILES,
        .checksum = MMAP_AUDIO_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config, &asset_audio);
    ESP_LOGI(TAG, "stored_files:%d", mmap_assets_get_stored_files(asset_audio));
}


static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    if (event == BUTTON_PRESS_DOWN) {
        ESP_LOGI(TAG, "Button pressed");

        if (play_mode_g == PLAY_MODE_OFF) { 
            play_mode_g = PLAY_MODE_ONESHOT;
            audio_mode_g = AUDIO_MODE_ONESHOT;
        } else {
            play_mode_g = PLAY_MODE_OFF;
            audio_mode_g = AUDIO_MODE_IDLE;
        }
    }
}

static void button_long_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    if (event == BUTTON_LONG_PRESS_START) {
        ESP_LOGI(TAG, "Button long pressed");
        play_mode_g = PLAY_MODE_CONTINUOUS;
        audio_mode_g = AUDIO_MODE_CONTINUOUS;
    }
}

static void display_task(void *arg)
{
    lv_disp_t *disp = bsp_display_start();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_90);

    image_mmap_init();

    static lv_img_dsc_t img_dsc_motive_A;

    mmap_assets_handle_t mmap_handle_A  = asset_DriverA_handle;

    ESP_LOGI("example", "Display LVGL animation");
    bsp_display_lock(0);

    lv_obj_t *obj_img_run_particles_A = lv_img_create(lv_scr_act());
    lv_obj_set_align(obj_img_run_particles_A, LV_ALIGN_CENTER);
    lv_img_set_src(obj_img_run_particles_A, &img_dsc_motive_A);

    bsp_display_unlock();

    while(1) {
        static uint32_t index = 0;

        bsp_display_lock(0);
        img_dsc_motive_A.data_size = mmap_assets_get_size(mmap_handle_A, index);
        img_dsc_motive_A.data = mmap_assets_get_mem(mmap_handle_A, index);
        lv_img_set_src(obj_img_run_particles_A, &img_dsc_motive_A);
        lv_refr_now(NULL);
        bsp_display_unlock();

        switch (play_mode_g) {
            case PLAY_MODE_OFF:
                // Reset to first frame when animation is paused and not already at first frame
                index = 0;
                break;

            case PLAY_MODE_ONESHOT:
                index++;
                if (index >= max_size_A) {
                    index = 0;
                    play_mode_g = PLAY_MODE_OFF;
                }
                break;

            case PLAY_MODE_CONTINUOUS:
                index++;
                if (index >= max_size_A) {
                    index = 2;
                }
                break;

            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(animation_delay_g));
    }
}

#define CHUNK_SIZE 4096

static void play_audio_from_mmap(const void *asset, uint32_t mmap_id, esp_codec_dev_handle_t codec_dev)
{
    void *audio = (void *)mmap_assets_get_mem(asset, mmap_id);
    uint32_t len = mmap_assets_get_size(asset, mmap_id);

    uint8_t *buf = (uint8_t *)malloc(CHUNK_SIZE);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to malloc audio buffer");
        return;
    }

    uint8_t *p = (uint8_t *)audio;
    uint32_t remaining = len;

    while (remaining > 0) {
        uint32_t copy_len = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        memcpy(buf, p, copy_len);
        esp_codec_dev_write(codec_dev, buf, copy_len);
        p += copy_len;
        remaining -= copy_len;
    }

    free(buf);
}

static void audio_play_task(void *arg)
{
    audio_mmap_init();

    esp_codec_dev_handle_t spk_codec_dev = bsp_audio_codec_speaker_init();

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = 44100,
        .channel            = 2,
        .channel_mask       = 0,
        .bits_per_sample    = 16,
        .mclk_multiple      = 0,
    };

    esp_codec_dev_open(spk_codec_dev, &fs);
    esp_codec_dev_set_out_vol(spk_codec_dev, 100);
    if(AUDIO_PA_CTL_GPIO != GPIO_NUM_NC) {
        gpio_set_level(AUDIO_PA_CTL_GPIO, 1);
    }

    ESP_LOGI(TAG, "================ Starting audio loop ================");

    while (1) {
        switch (audio_mode_g) {
            case AUDIO_MODE_IDLE:
                break;

            case AUDIO_MODE_ONESHOT:
                play_audio_from_mmap(asset_audio, 0, spk_codec_dev);
                // audio_mode_g = AUDIO_MODE_IDLE;
                if (audio_mode_g != AUDIO_MODE_CONTINUOUS) {
                    audio_mode_g = AUDIO_MODE_IDLE;
                }
                break;

            case AUDIO_MODE_CONTINUOUS:
                play_audio_from_mmap(asset_audio, 0, spk_codec_dev);
                break;

            default: 
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BSP_BUTTON_BOOT,
        .active_level = 0,
    };
    button_handle_t btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn);
    assert(ret == ESP_OK);
    assert(btn != NULL);
    iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_long_event_cb, NULL);

    xTaskCreate(display_task, "display_task", 1024 * 5, NULL, 5, NULL);
    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, NULL, 15, NULL);
}
