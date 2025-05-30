#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "m5_dlight.h"
#include "paj7620u2.h"
#include "bme68x.h"
#include "bme_esp_interface.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#include "bsec2.h"

static const char *TAG = "sensor_test";

// I2C总线配置
#define I2C_MASTER_SCL_IO           1      // I2C时钟引脚
#define I2C_MASTER_SDA_IO           2      // I2C数据引脚
#define I2C_MASTER_NUM              I2C_NUM_0  // I2C端口号
#define I2C_MASTER_FREQ_HZ          400000  // I2C主频

#define SAMPLE_RATE BSEC_SAMPLE_RATE_LP

const uint8_t default_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {
    #include <../components/BSEC2/Bosch-BSEC2-Library/src/config/bme688/bme688_sel_33v_3s_4d/bsec_selectivity.txt>
};

static i2c_bus_handle_t i2c_bus_handle = NULL;

typedef struct {
    i2c_bus_handle_t i2c_bus;
} bme688_task_params_t;

uint32_t IRAM_ATTR millis() { return (uint32_t) (esp_timer_get_time() / 1000ULL); }

void bme688_test_task(void *pvParameter)
{
    ESP_LOGI(TAG, "BME688 test task started");
    
    bme688_task_params_t *params = (bme688_task_params_t *)pvParameter;
    if (params == NULL || params->i2c_bus == NULL) {
        ESP_LOGE(TAG, "Invalid task parameters");
        vTaskDelete(NULL);
        return;
    }

    struct bme68x_dev bme;
    int8_t rslt;
    struct bme68x_conf conf;
    struct bme68x_heatr_conf heatr_conf;
    struct bme68x_data data[3];
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;
    i2c_bus_device_handle_t dev_handle = NULL;

    /* Heater temperature in degree Celsius */
    uint16_t temp_prof[10] = { 200, 240, 280, 320, 360, 360, 320, 280, 240, 200 };

    /* Heating duration in milliseconds */
    uint16_t dur_prof[10] = { 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000 };

    /* Initialize BME688 */
    rslt = bme68x_interface_init(&bme, BME68X_I2C_INTF, params->i2c_bus, BME68X_I2C_ADDR_HIGH);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 interface init failed");
        vTaskDelete(NULL);
        return;
    }

    rslt = bme68x_init(&bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 init failed");
        bme68x_interface_deinit(&dev_handle);
        vTaskDelete(NULL);
        return;
    }

    /* Configure BME688 */
    rslt = bme68x_get_conf(&conf, &bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 get conf failed");
        goto cleanup;
    }

    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    conf.os_hum = BME68X_OS_16X;
    conf.os_pres = BME68X_OS_1X;
    conf.os_temp = BME68X_OS_2X;
    rslt = bme68x_set_conf(&conf, &bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 set conf failed");
        goto cleanup;
    }

    /* Configure heater */
    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp_prof = temp_prof;
    heatr_conf.heatr_dur_prof = dur_prof;
    heatr_conf.profile_len = 10;
    rslt = bme68x_set_heatr_conf(BME68X_SEQUENTIAL_MODE, &heatr_conf, &bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 set heater conf failed");
        goto cleanup;
    }

    /* Set operation mode */
    rslt = bme68x_set_op_mode(BME68X_SEQUENTIAL_MODE, &bme);
    if (rslt != BME68X_OK) {
        ESP_LOGE(TAG, "BME688 set op mode failed");
        goto cleanup;
    }

    ESP_LOGI(TAG, "Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Profile index, Measurement index");
    
    /* Main loop */
    while (1) {
        /* Calculate delay period in microseconds */
        del_period = bme68x_get_meas_dur(BME68X_SEQUENTIAL_MODE, &conf, &bme) + (heatr_conf.heatr_dur_prof[0] * 1000);
        vTaskDelay(pdMS_TO_TICKS(del_period/1000));
        bme.delay_us(del_period % 1000, bme.intf_ptr);

        time_ms = esp_timer_get_time() / 1000; // Convert to milliseconds

        rslt = bme68x_get_data(BME68X_SEQUENTIAL_MODE, data, &n_fields, &bme);
        if (rslt != BME68X_OK) {
            ESP_LOGE(TAG, "BME688 get data failed");
            goto cleanup;
        }

        /* Print data */
        for (uint8_t i = 0; i < n_fields; i++) {
            ESP_LOGI(TAG, "%u, %lu, %.2f, %.2f, %.2f, %.2f, 0x%x, %d, %d",
                    sample_count,
                    (long unsigned int)time_ms + (i * (del_period / 2000)),
                    data[i].temperature,
                    data[i].pressure,
                    data[i].humidity,
                    data[i].gas_resistance,
                    data[i].status,
                    data[i].gas_index,
                    data[i].meas_index);
            sample_count++;
        }
    }

cleanup:
    bme68x_interface_deinit(&dev_handle);
    free(params);
    vTaskDelete(NULL);
}

void IRAM_ATTR delay(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }
uint32_t IRAM_ATTR micros() { return (uint32_t) esp_timer_get_time(); }

void delay_microseconds_safe(uint32_t us) {  // avoids CPU locks that could trigger WDT or affect WiFi/BT stability
  uint32_t start = micros();

  const uint32_t lag = 5000;  // microseconds, specifies the maximum time for a CPU busy-loop.
                              // it must be larger than the worst-case duration of a delay(1) call (hardware tasks)
                              // 5ms is conservative, it could be reduced when exact BT/WiFi stack delays are known
  if (us > lag) {
    delay((us - lag) / 1000UL);  // note: in disabled-interrupt contexts delay() won't actually sleep
    while (micros() - start < us - lag)
      delay(1);  // in those cases, this loop allows to yield for BT/WiFi stack tasks
  }
  while (micros() - start < us)  // fine delay the remaining usecs
    ;
}

void delay_us(uint32_t period, void *intfPtr) {
  delay_microseconds_safe(period);
}


void checkBsecStatus(Bsec2 bsec)
{
    if (bsec.status < BSEC_OK)
    {
        ESP_LOGI(TAG, "BSEC error code : %d", bsec.status);
    }
    else if (bsec.status > BSEC_OK)
    {
        ESP_LOGI(TAG, "BSEC warning code : %d", bsec.status);
    }

    if (bsec.sensor.status < BME68X_OK)
    {
        ESP_LOGI(TAG, "BME68X error code : %d", bsec.sensor.status);
    }
    else if (bsec.sensor.status > BME68X_OK)
    {
        ESP_LOGI(TAG, "BME68X warning code : %d", bsec.sensor.status);
    }
}

/* Function wrappers for I2C communication */
static i2c_bus_device_handle_t g_bme_dev = NULL;

static int8_t read_bytes_wrapper(uint8_t a_register, uint8_t *data, uint32_t len, void *intfPtr) {
    return i2c_bus_read_bytes(g_bme_dev, a_register, len, data) == ESP_OK ? 0 : -1;
}

static int8_t write_bytes_wrapper(uint8_t a_register, const uint8_t *data, uint32_t len, void *intfPtr) {
    return i2c_bus_write_bytes(g_bme_dev, a_register, len, data) == ESP_OK ? 0 : -1;
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
    if (!outputs.nOutputs)
    {
        return;
    }

    ESP_LOGI(TAG, "BSEC outputs:\n\tTime stamp = %d", (int) (outputs.output[0].time_stamp / INT64_C(1000000)));
    for (uint8_t i = 0; i < outputs.nOutputs; i++)
    {
        const bsecData output  = outputs.output[i];
        switch (output.sensor_id)
        {
            case BSEC_OUTPUT_IAQ:
                ESP_LOGI(TAG, "\tIAQ = %f", output.signal);
                ESP_LOGI(TAG, "\tIAQ accuracy = %d", (int) output.accuracy);
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                ESP_LOGI(TAG, "\tTemperature = %f", output.signal);
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                ESP_LOGI(TAG, "\tPressure = %f", output.signal);
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                ESP_LOGI(TAG, "\tHumidity = %f", output.signal);
                break;
            case BSEC_OUTPUT_RAW_GAS:
                ESP_LOGI(TAG, "\tGas resistance = %f", output.signal);
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                ESP_LOGI(TAG, "\tStabilization status = %f", output.signal);
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                ESP_LOGI(TAG, "\tRun in status = %f", output.signal);
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                ESP_LOGI(TAG, "\tCompensated temperature = %f", output.signal);
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                ESP_LOGI(TAG, "\tCompensated humidity = %f", output.signal);
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                ESP_LOGI(TAG, "\tStatic IAQ = %f", output.signal);
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                ESP_LOGI(TAG, "\tCO2 Equivalent = %f", output.signal);
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                ESP_LOGI(TAG, "\tbVOC equivalent = %f", output.signal);
                break;
            case BSEC_OUTPUT_GAS_PERCENTAGE:
                ESP_LOGI(TAG, "\tGas percentage = %f", output.signal);
                break;
            case BSEC_OUTPUT_COMPENSATED_GAS:
                ESP_LOGI(TAG, "\tCompensated gas = %f", output.signal);
                break;
            default:
                break;
        }
    }
}

void bsec_test_task(void *pvParameter)
{
    ESP_LOGI(TAG, "BSEC test task started");
    
    i2c_bus_handle_t i2c_bus = (i2c_bus_handle_t)pvParameter;
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "Invalid I2C bus handle");
        vTaskDelete(NULL);
        return;
    }

    /* Create BME688 device handle */
    g_bme_dev = i2c_bus_device_create(i2c_bus, BME68X_I2C_ADDR_HIGH, 0);
    if (g_bme_dev == NULL) {
        ESP_LOGE(TAG, "Failed to create BME688 device handle");
        vTaskDelete(NULL);
        return;
    }

    /* Create an object of the class Bsec2 */
    Bsec2 envSensor;

    /* Desired subscription list of BSEC2 outputs */
    bsecSensor sensorList[] = {
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_RAW_TEMPERATURE,
            BSEC_OUTPUT_RAW_PRESSURE,
            BSEC_OUTPUT_RAW_HUMIDITY,
            BSEC_OUTPUT_RAW_GAS,
            BSEC_OUTPUT_STABILIZATION_STATUS,
            BSEC_OUTPUT_RUN_IN_STATUS,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
            BSEC_OUTPUT_STATIC_IAQ,
            BSEC_OUTPUT_CO2_EQUIVALENT,
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
            BSEC_OUTPUT_GAS_PERCENTAGE,
            BSEC_OUTPUT_COMPENSATED_GAS
    };

    /* Initialize the library and interfaces */
    if (!envSensor.begin(BME68X_I2C_INTF, read_bytes_wrapper, write_bytes_wrapper, delay_us, (void *)&g_bme_dev, millis)) {
        ESP_LOGE(TAG, "BSEC initialization failed");
        i2c_bus_device_delete(&g_bme_dev);
        vTaskDelete(NULL);
        return;
    }
    if (!envSensor.setConfig(default_config)) {
        checkBsecStatus(envSensor);
    }

	if (SAMPLE_RATE == BSEC_SAMPLE_RATE_ULP)
	{
		envSensor.setTemperatureOffset(TEMP_OFFSET_ULP);
	}
	else if (SAMPLE_RATE == BSEC_SAMPLE_RATE_LP)
	{
		envSensor.setTemperatureOffset(TEMP_OFFSET_LP);
	}


    /* Subscribe to the desired BSEC2 outputs */
    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP)) {
        ESP_LOGE(TAG, "BSEC subscription failed");
        i2c_bus_device_delete(&g_bme_dev);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "BSEC subscription success");

    /* Whenever new data is available call the newDataCallback function */
    envSensor.attachCallback(newDataCallback);
    
    /* Main loop */
    while (1) {
        if (!envSensor.run())
        {
            checkBsecStatus(envSensor);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    /* Cleanup */
    i2c_bus_device_delete(&g_bme_dev);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Sensor test application started");
    
    // I2C总线配置
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = I2C_MASTER_FREQ_HZ
        },
        .clk_flags = 0
    };
    
    // 创建I2C总线
    i2c_bus_handle = i2c_bus_create(I2C_MASTER_NUM, &conf);
    if (i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C bus create failed");
        return;
    }
    
    // 扫描I2C设备
    uint8_t device_addr[10];
    uint8_t device_num = i2c_bus_scan(i2c_bus_handle, device_addr, 10);
    ESP_LOGI(TAG, "Found %d I2C devices", device_num);
    for (int i = 0; i < device_num; i++) {
        ESP_LOGI(TAG, "Device address: 0x%02x", device_addr[i]);
    }
    
    // 初始化DLight传感器
    dlight_init(i2c_bus_handle, DLIGHT_I2C_ADDR);

    // 设置高分辨率模式
    dlight_set_mode(DLIGHT_CONTINUOUSLY_H_RESOLUTION_MODE);

    paj7620_dev_t paj7620_dev;
    paj7620_init(&paj7620_dev, i2c_bus_handle);
    paj7620_set_high_rate(&paj7620_dev, true);

    // 创建BME688任务参数
    bme688_task_params_t *bme688_params = (bme688_task_params_t *)malloc(sizeof(bme688_task_params_t));
    if (bme688_params == NULL) {
        ESP_LOGE(TAG, "Failed to allocate task parameters");
        return;
    }
    bme688_params->i2c_bus = i2c_bus_handle;

    // BME688任务,测试获取原始的BME688数据
    // xTaskCreate(bme688_test_task, "bme688_task", 4096, bme688_params, 5, NULL);

    // BSEC测试任务,测试获取算法处理后的BSEC数据
    xTaskCreate(bsec_test_task, "bsec_task", 8192, i2c_bus_handle, 5, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        paj7620_gesture_t gesture = paj7620_get_gesture(&paj7620_dev);
        ESP_LOGI(TAG, "Gesture: %s", paj7620_get_gesture_desc(gesture));

        uint16_t lux;
        if (dlight_get_lux(&lux) == ESP_OK) {
            ESP_LOGI(TAG, "Light intensity: %d lux", lux);
        }
    }
} 