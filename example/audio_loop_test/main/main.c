/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "iot_button.h"
#include "button_gpio.h"

#define SAMPLE_RATE 16000
#define MAX_RECORD_TIME (3.0f)  // Maximum recording time in seconds
static const char *TAG = "audio_loop";

static TaskHandle_t audio_play_task_handle = NULL;
static TaskHandle_t audio_record_task_handle = NULL;
static bool is_recording = false;
static uint32_t recorded_samples = 0;

static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event(arg);
    if (event == BUTTON_PRESS_DOWN) {
        ESP_LOGI(TAG, "Button pressed - Start recording");
        is_recording = true;
        recorded_samples = 0;
        // Notify record task to start recording
        xTaskNotify(audio_record_task_handle, 0x01, eSetBits);
    }
    else if (event == BUTTON_PRESS_UP) {
        ESP_LOGI(TAG, "Button released - Stop recording and start playback");
        is_recording = false;
        // Notify play task to start playback
        xTaskNotify(audio_play_task_handle, 0x01, eSetBits);
    }
}

static void audio_play_task(void *arg)
{
    uint8_t *audio_buffer = (uint8_t *)arg;
    uint32_t ulNotificationValue;

    esp_codec_dev_handle_t spk_codec_dev = bsp_audio_codec_speaker_init();

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = SAMPLE_RATE,
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

    while (1) {
        // Wait for notification to start playback
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Start playback, recorded samples: %ld", recorded_samples);
            // Play recorded audio            
            esp_codec_dev_write(spk_codec_dev, audio_buffer, recorded_samples * sizeof(uint16_t));
            ESP_LOGI(TAG, "Playback finished");
        }
    }

    vTaskDelete(NULL);
}

static void audio_record_task(void *arg)
{
    uint8_t *audio_buffer = (uint8_t *)arg;
    uint32_t ulNotificationValue;
    esp_codec_dev_handle_t mic_codec_dev = bsp_audio_codec_microphone_init();
    if (mic_codec_dev == NULL) {
        ESP_LOGE(TAG, "Failed to init microphone codec");
        vTaskDelete(NULL);
        return;
    }
    esp_codec_dev_sample_info_t mic_fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(mic_codec_dev, &mic_fs);
    
    while(1) {
        // Wait for notification to start recording
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &ulNotificationValue, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Start recording");
            recorded_samples = 0;

            // Record until button is released or max time reached
            while (is_recording && recorded_samples < (MAX_RECORD_TIME * SAMPLE_RATE)) {
                // Read one second of audio at a time
                uint32_t samples_to_read = SAMPLE_RATE / 10;  //every 100ms
                if (recorded_samples + samples_to_read > (MAX_RECORD_TIME * SAMPLE_RATE)) {
                    samples_to_read = (MAX_RECORD_TIME * SAMPLE_RATE) - recorded_samples;
                }
                
                esp_codec_dev_read(mic_codec_dev, 
                                 audio_buffer + (recorded_samples * sizeof(uint16_t)), 
                                 samples_to_read * sizeof(uint16_t));
                recorded_samples += samples_to_read;
                
                ESP_LOGI(TAG, "Recorded %ld samples", recorded_samples);
            }
            
            ESP_LOGI(TAG, "Recording finished, total samples: %ld", recorded_samples);
        }
    }
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
    iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);

    // Allocate buffer for maximum recording time
    uint8_t *audio_buffer = malloc(MAX_RECORD_TIME * SAMPLE_RATE * sizeof(uint16_t));
    if (audio_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
        return;
    }

    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, audio_buffer, 15, &audio_play_task_handle);
    xTaskCreate(audio_record_task, "audio_record_task", 1024 * 5, audio_buffer, 10, &audio_record_task_handle);
}
