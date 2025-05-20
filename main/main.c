#include "lvgl_init.h"
#include "ui.h"
#include "dht11.h"
#include "esp_log.h"
#include "mq2.h"
#include "wifi.h"
#include "mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "led_strip.h"

#define DHT11_GPIO 4
#define LED_STRIP_GPIO_PIN 32
#define LED_STRIP_LED_COUNT 19
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

#define LIGHT_POWER_EVENT_BIT BIT0

static const char *TAG = "main";
static void mq2_task(void *pvParameter);
static void dht11_task(void *pvParameter);
static void ws2812_task(void *pvParameter);
static void init_queue();
static void init_ws2812();

QueueHandle_t Queue_mqtt_airquality_lvgl;
QueueHandle_t Queue_mq2_mqtt;
QueueHandle_t Queue_dht11_lvgl;
QueueHandle_t Queue_dht11_mqtt;
QueueHandle_t Queue_mqtt_weather_lvgl;
QueueHandle_t Queue_mqtt_time_lvgl;
QueueHandle_t Queue_mqtt_date_lvgl;
QueueHandle_t Queue_lvgl_volume_mqtt;
// QueueHandle_t Queue_mqtt_switch_light_lvgl;
// QueueHandle_t Queue_mqtt_switch_wet_lvgl;
// QueueHandle_t Queue_mqtt_switch_cold_lvgl;
// QueueHandle_t Queue_mqtt_volumn_lvgl;

SemaphoreHandle_t mqtt_connect_smphr;

SemaphoreHandle_t rmt_smphr;

static led_strip_handle_t ws2812_handle;

status_t status = {
    .volume = 0,
    .light.brightness = 255,
    .light.color = {44, 211, 111},
    .light.power = false,
    .light.mode = 0,
    .light.final_color = {44, 211, 111},
    .wet = false,
    .cold = false,
};

void app_main(void)
{
    init_wifi();
    mqtt_connect_smphr = xSemaphoreCreateBinary();
    rmt_smphr = xSemaphoreCreateMutex();
    init_queue();

    ESP_ERROR_CHECK(app_lcd_init());

    ESP_ERROR_CHECK(app_touch_init());

    ESP_LOGI(TAG, "Touch initialized successfully");

    ESP_ERROR_CHECK(app_lvgl_init());

    init_MQ2();
    DHT11_Init(DHT11_GPIO);
    init_ws2812();

    xTaskCreatePinnedToCore(dht11_task, "dht11_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(mq2_task, "mq2_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(ws2812_task, "ws2812_task", 4096, NULL, 5, NULL, 1);

    app_main_display();

    xSemaphoreTake(mqtt_connect_smphr, portMAX_DELAY);
    init_mqtt();

    xTaskCreatePinnedToCore(mqtt_publish_dht11, "mqtt_publish_dht11", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(mqtt_publish_mq2, "mqtt_publish_mq2", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(mqtt_publish_volumn, "mqtt_publish_volumn", 4096, NULL, 5, NULL, 1);
}

void init_queue()
{
    Queue_mq2_mqtt = xQueueCreate(10, sizeof(float));
    Queue_dht11_lvgl = xQueueCreate(10, sizeof(dht11_t));
    Queue_dht11_mqtt = xQueueCreate(10, sizeof(dht11_t));
    Queue_mqtt_weather_lvgl = xQueueCreate(10, sizeof(char) * 10);
    Queue_mqtt_airquality_lvgl = xQueueCreate(10, sizeof(char) * 10);
    Queue_mqtt_time_lvgl = xQueueCreate(2, sizeof(char) * 10);
    Queue_mqtt_date_lvgl = xQueueCreate(2, sizeof(char) * 15);
    Queue_lvgl_volume_mqtt = xQueueCreate(2, sizeof(int));
}

void init_ws2812(void)
{
    led_strip_config_t strip_config = {
        .max_leds = LED_STRIP_LED_COUNT,                             // The number of LEDs in the strip,
        .strip_gpio_num = LED_STRIP_GPIO_PIN,                        // The GPIO that connected to the LED strip's data line
        .led_model = LED_MODEL_WS2812,                               // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = 64,               // the memory size of each RMT channel, in words (4 bytes)
        .flags = {
            .with_dma = false,
        },
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &ws2812_handle));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
}

void dht11_task(void *pvParameter)
{
    while (1)
    {
        dht11_t dht11_data;
        // xSemaphoreTake(rmt_smphr, portMAX_DELAY);
        if (DHT11_StartGet(&dht11_data))
        {
            xQueueSend(Queue_dht11_lvgl, &dht11_data, pdMS_TO_TICKS(10));
            xQueueSend(Queue_dht11_mqtt, &dht11_data, pdMS_TO_TICKS(10));
            ESP_LOGI(TAG, "Temperature: %.1f, Humidity: %d%%", dht11_data.temp, dht11_data.humidity);
        }
        // xSemaphoreGive(rmt_smphr);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void mq2_task(void *pvParameter)
{

    while (1)
    {
        int sensorvalue; // 读取模拟值
        get_MQ2_data(&sensorvalue);
        float voltage = sensorvalue * (3.3 / 4095.0); // 转换为电压

        ESP_LOGI(TAG, "voltage: %f", voltage);

        float rs = ((3.3 - voltage) * RL) / voltage; // 计算rs
        ESP_LOGI(TAG, "rs: %f", rs);

        float ppm = calculatePPM(rs); // 计算浓度
        ESP_LOGI(TAG, "ppm: %f", ppm);

        xQueueSend(Queue_mq2_mqtt, &ppm, pdMS_TO_TICKS(10));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void ws2812_task(void *pvParameter)
{
    uint8_t temp[3] = {0};
    int16_t temp_brightness = status.light.brightness;
    int8_t temp_brightness_change = -2;
    while (1)
    {
        ESP_LOGI(TAG, "light: %d", status.light.power);
        ESP_LOGI(TAG, "light mode: %d", status.light.mode);
        if (status.light.power)
        {
            if (status.light.mode == 0)
            {
                for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
                {
                    ESP_ERROR_CHECK(led_strip_set_pixel(ws2812_handle, i, status.light.final_color[0], status.light.final_color[1], status.light.final_color[2]));
                }
                ESP_ERROR_CHECK(led_strip_refresh(ws2812_handle));
            }
            else if (status.light.mode == 1)
            {
                temp[0] = (status.light.color[0] * temp_brightness) >> 8;
                temp[1] = (status.light.color[1] * temp_brightness) >> 8;
                temp[2] = (status.light.color[2] * temp_brightness) >> 8;
                for (int i = 0; i < LED_STRIP_LED_COUNT; i++)
                {
                    ESP_ERROR_CHECK(led_strip_set_pixel(ws2812_handle, i, temp[0], temp[1], temp[2]));
                }
                ESP_ERROR_CHECK(led_strip_refresh(ws2812_handle));
                temp_brightness += temp_brightness_change;
                if (temp_brightness > status.light.brightness || temp_brightness < 0)
                {
                    temp_brightness = (temp_brightness > status.light.brightness) ? status.light.brightness : 0;

                    temp_brightness_change = -temp_brightness_change;
                }
            }
        }
        else
        {
            ESP_ERROR_CHECK(led_strip_clear(ws2812_handle));
            ESP_ERROR_CHECK(led_strip_refresh(ws2812_handle));
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
