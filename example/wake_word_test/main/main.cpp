/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_lv_fs.h"
#include "esp_lv_decoder.h"
#include "mmap_generate_gif.h"
#include "mmap_generate_audio.h"

#include "iot_button.h"
#include "button_gpio.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"

#include <sys/time.h>

#include "model_path.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "led_strip.h"
#include <math.h>

#define SAMPLE_RATE 16000
#define SAMPLE_TIME (0.03f)
static const char *TAG = "main";

static TaskHandle_t audio_play_task_handle = NULL;
static mmap_assets_handle_t asset_DriverA_handle = NULL;
static mmap_assets_handle_t asset_audio          = NULL;

static esp_lv_fs_handle_t fs_DriverA_handle      = NULL;
static esp_lv_decoder_handle_t decoder_handle    = NULL;

static led_strip_handle_t led_strip = NULL;

// Define queue handle for audio data
static QueueHandle_t audio_queue = NULL;

// Define audio chunk structure
typedef struct {
    int16_t *buffer;
    size_t size;
} audio_chunk_t;

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
            .mmap_enable = true,
            .app_bin_check = true,
            .full_check = false,
            .metadata_check = false,
            .reserved = 0
        }
    };
    ESP_ERROR_CHECK(mmap_assets_new(&config_DriveA, &asset_DriverA_handle));

    const fs_cfg_t fs_cfg_a = {
        .fs_letter = 'A',
        .fs_nums = MMAP_GIF_FILES,
        .fs_assets = asset_DriverA_handle
    };
    esp_lv_fs_desc_init(&fs_cfg_a, &fs_DriverA_handle);

    esp_lv_decoder_init(&decoder_handle); //Initialize this after lvgl starts
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = 8,
        .max_leds = 4, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false,
        },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

static void set_led(uint8_t r, uint8_t g, uint8_t b)
{
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_set_pixel(led_strip, 1, r, g, b);
    led_strip_set_pixel(led_strip, 2, r, g, b);
    led_strip_set_pixel(led_strip, 3, r, g, b);
    led_strip_refresh(led_strip);
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
            .full_check = false,
            .metadata_check = false,
            .reserved = 0
        }
    };

    mmap_assets_new(&config, &asset_audio);
    ESP_LOGI(TAG, "stored_files:%d", mmap_assets_get_stored_files(asset_audio));
}


static void button_event_cb(void *arg, void *data)
{
    button_event_t event = iot_button_get_event((button_handle_t)arg);
    if (event == BUTTON_PRESS_DOWN) {
        ESP_LOGI(TAG, "Button pressed - Start recording");
        // Notify record task to start recording
        // xTaskNotify(audio_record_task_handle, 0x01, eSetBits);
    }
    else if (event == BUTTON_PRESS_UP) {
        ESP_LOGI(TAG, "Button released - Start playback");
        // Notify play task to start playback
        // xTaskNotify(audio_play_task_handle, 0x01, eSetBits);
    }
}

/**
 * Convert HSL color space to RGB color space
 * 
 * @param h Hue value (0-360)
 * @param s Saturation value (0-100)
 * @param l Lightness value (0-100)
 * @param r Red component output (0-255)
 * @param g Green component output (0-255)
 * @param b Blue component output (0-255)
 */
static void hsl_to_rgb(float h, float s, float l, uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Convert saturation and lightness to range 0-1
    s /= 100.0f;
    l /= 100.0f;

    float c = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r_temp = 0, g_temp = 0, b_temp = 0;

    if (h >= 0 && h < 60) {
        r_temp = c;
        g_temp = x;
    } else if (h >= 60 && h < 120) {
        r_temp = x;
        g_temp = c;
    } else if (h >= 120 && h < 180) {
        g_temp = c;
        b_temp = x;
    } else if (h >= 180 && h < 240) {
        g_temp = x;
        b_temp = c;
    } else if (h >= 240 && h < 300) {
        r_temp = x;
        b_temp = c;
    } else {
        r_temp = c;
        b_temp = x;
    }

    // Convert to RGB range 0-255
    *r = (uint8_t)((r_temp + m) * 255.0f);
    *g = (uint8_t)((g_temp + m) * 255.0f);
    *b = (uint8_t)((b_temp + m) * 255.0f);
}

static void display_task(void *arg)
{
    lv_disp_t *disp = bsp_display_start();
    bsp_display_rotate(disp, LV_DISPLAY_ROTATION_90);

    image_mmap_init();

    static lv_img_dsc_t img_dsc_motive_A;

    mmap_assets_handle_t mmap_handle_A  = asset_DriverA_handle;

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

        if(play_mode_g != PLAY_MODE_OFF){
            uint8_t r, g, b;
            hsl_to_rgb(index * 360 / max_size_A, 100, 25, &r, &g, &b);
            bsp_led_rgb_set(r, g, b);
        }
        else{
            bsp_led_rgb_set(0, 0, 0);
        }
        
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
    void *audio = (void *)mmap_assets_get_mem((mmap_assets_handle_t)asset, mmap_id);
    uint32_t len = mmap_assets_get_size((mmap_assets_handle_t)asset, mmap_id);

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
        .bits_per_sample    = 16,
        .channel            = 2,
        .channel_mask       = 0,
        .sample_rate        = 44100,
        .mclk_multiple      = 0,
    };

    esp_codec_dev_open(spk_codec_dev, &fs);
    esp_codec_dev_set_out_vol(spk_codec_dev, 60);
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
                play_audio_from_mmap(asset_audio, 1, spk_codec_dev);
                break;

            default: 
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

static void audio_capture_task(void *arg)
{
    ESP_LOGI(TAG, "Starting audio capture task");
    
    esp_codec_dev_handle_t mic_codec_dev = bsp_audio_codec_microphone_init();
    if (mic_codec_dev == NULL) {
        ESP_LOGE(TAG, "Failed to init microphone codec");
        vTaskDelete(NULL);
        return;
    }
    
    esp_codec_dev_sample_info_t mic_fs = {
        .bits_per_sample = 16,
        .channel = 1,
        .channel_mask = 0,
        .sample_rate = SAMPLE_RATE,
        .mclk_multiple = 0
    };
    esp_codec_dev_open(mic_codec_dev, &mic_fs);
    
    // Get audio parameters from wake word model
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL) {
        ESP_LOGE(TAG, "Failed to initialize wake word model");
        vTaskDelete(NULL);
        return;
    }
    
    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    if (model_name == NULL) {
        ESP_LOGE(TAG, "Failed to get model name");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    esp_wn_iface_t *wakenet = (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
    if (wakenet == NULL) {
        ESP_LOGE(TAG, "Failed to get wake word interface");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    model_iface_data_t *model_data = wakenet->create(model_name, DET_MODE_95);
    if (model_data == NULL) {
        ESP_LOGE(TAG, "Failed to create model data");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    int audio_chunksize = wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);
    wakenet->destroy(model_data);
    esp_srmodel_deinit(models);
    
    if (audio_chunksize <= 0) {
        ESP_LOGE(TAG, "Invalid audio chunk size: %d", audio_chunksize);
        vTaskDelete(NULL);
        return;
    }
    
    int16_t *buffer = (int16_t *) malloc(audio_chunksize);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer of size %d", audio_chunksize);
        vTaskDelete(NULL);
        return;
    }
    
    while (1) {
        esp_codec_dev_read(mic_codec_dev, buffer, audio_chunksize);
        
        // Create audio chunk and send to queue
        audio_chunk_t chunk = {
            .buffer = (int16_t *)malloc(audio_chunksize),
            .size = (size_t)audio_chunksize  // Explicit cast to size_t
        };
        
        if (chunk.buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate chunk buffer");
            continue;
        }
        
        memcpy(chunk.buffer, buffer, audio_chunksize);
        
        if (xQueueSend(audio_queue, &chunk, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send audio chunk to queue");
            free(chunk.buffer);
        }
    }
    
    free(buffer);
    vTaskDelete(NULL);
}

static void wake_word_detection_task(void *arg)
{
    ESP_LOGI(TAG, "Starting wake word detection task");
    
    // Initialize wake word model
    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL) {
        ESP_LOGE(TAG, "Failed to initialize wake word model");
        vTaskDelete(NULL);
        return;
    }
    
    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    if (model_name == NULL) {
        ESP_LOGE(TAG, "Failed to get model name");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    esp_wn_iface_t *wakenet = (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
    if (wakenet == NULL) {
        ESP_LOGE(TAG, "Failed to get wake word interface");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    model_iface_data_t *model_data = wakenet->create(model_name, DET_MODE_95);
    if (model_data == NULL) {
        ESP_LOGE(TAG, "Failed to create model data");
        esp_srmodel_deinit(models);
        vTaskDelete(NULL);
        return;
    }
    
    audio_chunk_t chunk;
    while (1) {
        if (xQueueReceive(audio_queue, &chunk, portMAX_DELAY) == pdTRUE) {
            int res = wakenet->detect(model_data, chunk.buffer);
            if (res > 0) {
                ESP_LOGI(TAG, "Wake word detected!");
                if (play_mode_g == PLAY_MODE_OFF) { 
                    play_mode_g = PLAY_MODE_ONESHOT;
                    audio_mode_g = AUDIO_MODE_ONESHOT;
                } else {
                    play_mode_g = PLAY_MODE_OFF;
                    audio_mode_g = AUDIO_MODE_IDLE;
                }
            }
            free(chunk.buffer);
        }
    }
    
    wakenet->destroy(model_data);
    esp_srmodel_deinit(models);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    const button_config_t btn_cfg = {
        .short_press_time = 0,  // Add missing initializer
    };
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = BSP_BUTTON_BOOT,
        .active_level = 0,
        .enable_power_save = false,  // Add missing initializer
        .disable_pull = false,       // Add missing initializer
    };
    button_handle_t btn = NULL;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &btn);
    assert(ret == ESP_OK);
    assert(btn != NULL);
    iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_cb, NULL);
    iot_button_register_cb(btn, BUTTON_PRESS_UP, NULL, button_event_cb, NULL);

    bsp_led_init();
    
    // Create queue for audio data
    audio_queue = xQueueCreate(10, sizeof(audio_chunk_t));
    if (audio_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create audio queue");
        return;
    }
    
    // Create audio capture and wake word detection tasks
    xTaskCreate(audio_capture_task, "audio_capture", 1024 * 4, NULL, 5, NULL);
    xTaskCreate(wake_word_detection_task, "wake_word_detection", 1024 * 4, NULL, 5, NULL);
    
    xTaskCreate(display_task, "display_task", 1024 * 5, NULL, 5, NULL);
    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, NULL, 5, &audio_play_task_handle);
}
