/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "mmap_generate_audio.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define CHUNK_SIZE 4096

/** 
 * @brief Audio player state
 */
typedef enum {
    PLAYER_STATE_IDLE = 0,    /** Player is idle */
    PLAYER_STATE_PLAYING,     /** Player is playing */
    PLAYER_STATE_PAUSED,      /** Player is paused */
} player_state_t;

static const char *TAG = "music_player";
static mmap_assets_handle_t asset_audio = NULL;
static SemaphoreHandle_t audio_sem = NULL;

static player_state_t player_state = PLAYER_STATE_IDLE;
static uint32_t current_audio_index = 0;
static bool should_play_next = false;

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
        
        switch (player_state) {
            case PLAYER_STATE_IDLE:
                player_state = PLAYER_STATE_PLAYING;
                should_play_next = false;
                break;
                
            case PLAYER_STATE_PLAYING:
                player_state = PLAYER_STATE_PAUSED;
                break;
                
            case PLAYER_STATE_PAUSED:
                player_state = PLAYER_STATE_PLAYING;
                should_play_next = true;
                break;
        }
        
        // Notify audio task
        xSemaphoreGive(audio_sem);
    }
}

static void play_audio_from_mmap(void *asset, uint32_t mmap_id, esp_codec_dev_handle_t codec_dev)
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

    while (remaining > 0 && player_state == PLAYER_STATE_PLAYING) {
        uint32_t copy_len = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        memcpy(buf, p, copy_len);
        esp_codec_dev_write(codec_dev, buf, copy_len);
        p += copy_len;
        remaining -= copy_len;
    }

    free(buf);
    
    // Set state to PAUSED after playback completes
    if (player_state == PLAYER_STATE_PLAYING) {
        player_state = PLAYER_STATE_PAUSED;
        ESP_LOGI(TAG, "Audio playback completed, state changed to PAUSED");
    }
}

static void audio_play_task(void *arg)
{
    audio_mmap_init();

    esp_codec_dev_handle_t spk_codec_dev = bsp_audio_codec_speaker_init();

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = 44100,
        .channel            = 1,
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
        // Wait for semaphore from button callback
        if (xSemaphoreTake(audio_sem, portMAX_DELAY) == pdTRUE) {
            switch (player_state) {
                case PLAYER_STATE_PLAYING:
                    if (should_play_next) {
                        current_audio_index = (current_audio_index + 1) % MMAP_AUDIO_FILES;
                        ESP_LOGI(TAG, "Playing next audio file: %ld", current_audio_index);
                    }
                    ESP_LOGI(TAG, "Playing audio file: %ld", current_audio_index);
                    play_audio_from_mmap(asset_audio, current_audio_index, spk_codec_dev);
                    break;
                    
                case PLAYER_STATE_PAUSED:
                    ESP_LOGI(TAG, "Audio paused");
                    break;
                    
                default:
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    // Create binary semaphore
    audio_sem = xSemaphoreCreateBinary();
    if (audio_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }

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

    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, NULL, 15, NULL);
}
