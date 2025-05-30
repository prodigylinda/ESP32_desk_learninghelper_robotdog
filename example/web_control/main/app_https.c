/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "app_servo_control.h"
#include "servo_dog_ctrl.h"
#include "mdns.h"

#define TAG "HTTP_SERVER"

static bool is_calibration_mode = false;

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

#define STATIC_PATH "/spiffs"

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static esp_err_t static_file_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    char filepath[1024];

    if (strcmp(uri, "/") == 0) {
        uri = "/index.html";
    }
    snprintf(filepath, sizeof(filepath), "/spiffs%s", uri);
    ESP_LOGI(TAG, "filepath: %s", filepath);
    FILE *file = fopen(filepath, "r");

    if (!file) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char buffer[1024];
    size_t read_bytes;
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// API: GET /api/404
static esp_err_t get_404_handler(httpd_req_t *req)
{
    httpd_resp_send_404(req);
    return ESP_OK;
}

// API: POST /reset
static esp_err_t start_calibration_handler_func(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Start calibration request received");
    char response[128];
    int fl = 0, fr = 0, bl = 0, br = 0;
    servo_control_get_save_value(&fl, &fr, &bl, &br);
    ESP_LOGI(TAG, "FL: %d, FR: %d, BL: %d, BR: %d", fl, fr, bl, br);
    int len = snprintf(response, sizeof(response), "{\"fl\":%d,\"fr\":%d,\"bl\":%d,\"br\":%d}", fl, fr, bl, br);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, len);
    servo_dog_ctrl_send(DOG_STATE_INSTALLATION, NULL);
    is_calibration_mode = true;
    return ESP_OK;
}

static esp_err_t exit_calibration_handler_func(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Exit calibration request received");
    servo_dog_ctrl_send(DOG_STATE_IDLE, NULL);
    is_calibration_mode = false;
    httpd_resp_send(req, "{\"code\":200}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t adjust_handler_func(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Adjust request received");
    char content[128] = {0};
    int ret = httpd_req_recv(req, content, sizeof(content));
    ESP_LOGI(TAG, "Content: %s", content);
    char leg_str[16] = {0};
    int leg_value = 0;

    int fl = 0, fr = 0, bl = 0, br = 0;
    servo_control_get_save_value(&fl, &fr, &bl, &br);
    // {"servo":"bl","value":1}
    sscanf(content, "{\"servo\":\"%[^\"]\",\"value\":%d}", leg_str, &leg_value);
    if (strcmp(leg_str, "fl") == 0) {
        ESP_LOGI(TAG, "Front left: %d", leg_value);
        fl = leg_value;
    } else if (strcmp(leg_str, "fr") == 0) {
        ESP_LOGI(TAG, "Front right: %d", leg_value);
        fr = leg_value;
    } else if (strcmp(leg_str, "bl") == 0) {
        ESP_LOGI(TAG, "Back left: %d", leg_value);
        bl = leg_value;
    } else if (strcmp(leg_str, "br") == 0) {
        ESP_LOGI(TAG, "Back right: %d", leg_value);
        br = leg_value;
    }

    servo_control_set_save_value(fl, fr, bl, br);
    servo_dog_ctrl_send(DOG_STATE_INSTALLATION, NULL);
    httpd_resp_send(req, "{\"code\":200}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// API: POST /control
static esp_err_t control_handler_func(httpd_req_t *req)
{
    if (is_calibration_mode) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\": \"Control disabled in calibration mode\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Control request received");
    char content[128] = {0};
    int ret = httpd_req_recv(req, content, sizeof(content));
    ESP_LOGI(TAG, "Content: %s", content);

    char function[16];
    char value[16];
    // {"%s":"%s"}
    sscanf(content, "{\"%[^\"]\":\"%[^\"]\"}", function, value);
    if (strcmp(function, "move") == 0) {
        ESP_LOGI(TAG, "Move request received");
        switch (value[0]) {
            case 'F':
                ESP_LOGI(TAG, "Forward");
                servo_dog_ctrl_send(DOG_STATE_FORWARD, NULL);
                break;
            case 'B':
                ESP_LOGI(TAG, "Backward");
                servo_dog_ctrl_send(DOG_STATE_BACKWARD, NULL);
                break;
            case 'L':
                ESP_LOGI(TAG, "Left");
                servo_dog_ctrl_send(DOG_STATE_TURN_LEFT, NULL);
                break;
            case 'R':
                ESP_LOGI(TAG, "Right");
                servo_dog_ctrl_send(DOG_STATE_TURN_RIGHT, NULL);
                break;
        }
    } else if (strcmp(function, "action") == 0) {
        ESP_LOGI(TAG, "Action request received %s", value);
        servo_dog_ctrl_send(atoi(value) + DOG_STATE_TURN_LEFT, NULL);
    }
    httpd_resp_send(req, "{\"code\":200}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 1024 * 6;
    config.task_priority = 10;
    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    // HTTP_GET /start_calibration
    httpd_uri_t start_calibration_handler = {
        .uri = "/start_calibration",
        .method = HTTP_GET,
        .handler = start_calibration_handler_func,
        .user_ctx = NULL
    };

    // HTTP_GET /exit_calibration
    httpd_uri_t exit_calibration_handler = {
        .uri = "/exit_calibration",
        .method = HTTP_GET,
        .handler = exit_calibration_handler_func,
        .user_ctx = NULL
    };

    // POST /adjust
    httpd_uri_t adjust_handler = {
        .uri = "/adjust",
        .method = HTTP_POST,
        .handler = adjust_handler_func,
        .user_ctx = NULL
    };

    // Post /control
    httpd_uri_t control_handler = {
        .uri = "/control",
        .method = HTTP_POST,
        .handler = control_handler_func,
        .user_ctx = NULL
    };

    httpd_uri_t static_handler = {
        .uri = "*",
        .method = HTTP_GET,
        .handler = static_file_handler,
        .user_ctx = NULL
    };

    httpd_handle_t server = NULL;

    config.core_id = 0;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &start_calibration_handler);
        httpd_register_uri_handler(server, &exit_calibration_handler);
        httpd_register_uri_handler(server, &adjust_handler);
        httpd_register_uri_handler(server, &control_handler);
        httpd_register_uri_handler(server, &static_handler);
    } else {
        goto exit;
    }

    return ESP_OK;
exit:
    return ESP_FAIL;
}

void start_mdns_service(void) {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS init failed");
        return;
    }
    err = mdns_hostname_set(CONFIG_ESP_MDNS_HOSTNAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS hostname set failed");
        return;
    }
    err = mdns_instance_name_set("ESP-Hi Web Control");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS instance name set failed");
        return;
    }
    err = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS service add failed");
        return;
    }
    err = mdns_service_instance_name_set("_http", "_tcp", "ESP-Hi Web Control");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS service instance name set failed");
        return;
    }
    ESP_LOGI(TAG, "MDNS service started");
}

void app_https_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = STATIC_PATH,
        .partition_label = "storage",
        .max_files = 5,   // This decides the maximum number of files that can be created on the storage
        .format_if_mount_failed = false
    };

    start_mdns_service();

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
    ESP_ERROR_CHECK(start_webserver());
}
