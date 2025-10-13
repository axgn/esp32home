#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include "dht11.h"
#include "umqtt.h"

#define MQTT_HOST "MQTT_HOST"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "mqtt_esp32_123"
#define MQTT_USERNAME "MQTT_USERNAME"
#define MQTT_PASSWORD "MQTT_PASSWORD"

#define ESP32_PUB_TEMPERATURE_TOPIC "esp32/temperature"
#define ESP32_PUB_HUMIDITY_TOPIC "esp32/humidity"
#define ESP32_PUB_SMOKE_TOPIC "esp32/smokeConcentration"
#define ESP32_REC_LIGHT_POWER_TOPIC "esp32s3/light_power"
#define ESP32_REC_LIGHT_MODE_TOPIC "esp32s3/light_mode"
#define ESP32_REC_LIGHT_COLOR_TOPIC "esp32s3/light_color"
#define ESP32_REC_LIGHT_BRIGHTNESS_TOPIC "esp32s3/light_brightness"
#define ESP32_REC_HUMIDIFIER_TOPIC "server/humidifier"
#define ESP32_REC_AIR_CONDITIONER_TOPIC "server/airConditioner"
#define ESP32_REC_VOLUME_TOPIC "server/volume"
#define ESP32_REC_WEATHER_TOPIC "server/weather"
#define ESP32_REC_TIME_TOPIC "server/time"
#define ESP32_REC_DATE_TOPIC "server/date"
#define ESP32_REC_AQI_TOPIC "server/AQI"
#define ESP32_PUB_VOLUME_TOPIC "esp32/volume"

#define LIGHT_POWER_EVENT_BIT BIT0

esp_mqtt_client_handle_t mqtt_handle = NULL;

static const char *TAG = "MQTT";

static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void deal_data(esp_mqtt_event_handle_t data);
static void esp_mqtt_subscribe();
static void calculate_light_color();

void init_mqtt()
{
    esp_mqtt_client_config_t mqtt_cfg = {0};

    mqtt_cfg.broker.address.uri = MQTT_HOST;
    mqtt_cfg.broker.address.port = MQTT_PORT;
    mqtt_cfg.credentials.client_id = MQTT_CLIENT_ID;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_handle);

    esp_mqtt_client_start(mqtt_handle);
}

void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t data = (esp_mqtt_event_handle_t)event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        esp_mqtt_subscribe();
        ESP_LOGI(TAG, "mqtt connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "mqtt disconnected");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "mqtt subscribed success");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "mqtt published success");
        break;
    case MQTT_EVENT_DATA:
        deal_data(data);
        break;
    default:
        break;
    }
}

void esp_mqtt_subscribe()
{
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_LIGHT_POWER_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_LIGHT_MODE_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_LIGHT_COLOR_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_LIGHT_BRIGHTNESS_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_VOLUME_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_WEATHER_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_TIME_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_DATE_TOPIC, 1);
    esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_AQI_TOPIC, 1);
    // esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_HUMIDIFIER_TOPIC, 1);
    // esp_mqtt_client_subscribe_single(mqtt_handle, ESP32_REC_AIR_CONDITIONER_TOPIC, 1);
}

void deal_data(esp_mqtt_event_handle_t data)
{
    char topic[25], payload[15];
    memcpy(topic, data->topic, data->topic_len);
    memcpy(payload, data->data, data->data_len);
    topic[data->topic_len] = '\0';
    payload[data->data_len] = '\0';
    if (strcmp(topic, ESP32_REC_WEATHER_TOPIC) == 0)
    {
        xQueueSend(Queue_mqtt_weather_lvgl, payload, pdMS_TO_TICKS(10));
    }
    else if (strcmp(topic, ESP32_REC_TIME_TOPIC) == 0)
    {
        xQueueSend(Queue_mqtt_time_lvgl, payload, pdMS_TO_TICKS(10));
    }
    else if (strcmp(topic, ESP32_REC_DATE_TOPIC) == 0)
    {
        xQueueSend(Queue_mqtt_date_lvgl, payload, pdMS_TO_TICKS(10));
    }
    else if (strcmp(topic, ESP32_REC_AQI_TOPIC) == 0)
    {
        xQueueSend(Queue_mqtt_airquality_lvgl, payload, pdMS_TO_TICKS(10));
    }
    else if (strcmp(topic, ESP32_REC_LIGHT_POWER_TOPIC) == 0)
    {
        status.light.power = (strcmp(payload, "on") == 0) ? true : false;
    }
    else if (strcmp(topic, ESP32_REC_HUMIDIFIER_TOPIC) == 0)
    {
        status.wet = (strcmp(payload, "on") == 0) ? true : false;
    }
    else if (strcmp(topic, ESP32_REC_AIR_CONDITIONER_TOPIC) == 0)
    {
        status.cold = (strcmp(payload, "on") == 0) ? true : false;
    }
    else if (strcmp(topic, ESP32_REC_VOLUME_TOPIC) == 0)
    {
        status.volume = (payload[0] - '0') * 100 + (payload[1] - '0') * 10 + (payload[2] - '0');
    }
    else if (strcmp(topic, ESP32_REC_LIGHT_MODE_TOPIC) == 0)
    {
        status.light.mode = (payload[0] - '0');
    }
    else if (strcmp(topic, ESP32_REC_LIGHT_COLOR_TOPIC) == 0)
    {
        sscanf(payload, "%hhu,%hhu,%hhu", &status.light.color[0], &status.light.color[1], &status.light.color[2]);
        ESP_LOGI(TAG, "light color123: %d %d %d", status.light.color[0], status.light.color[1], status.light.color[2]);
        // status.light.color[0] = (payload[0] - '0') * 100 + (payload[1] - '0') * 10 + (payload[2] - '0');
        // status.light.color[1] = (payload[4] - '0') * 100 + (payload[5] - '0') * 10 + (payload[6] - '0');
        // status.light.color[2] = (payload[8] - '0') * 100 + (payload[9] - '0') * 10 + (payload[10] - '0');
        calculate_light_color();
    }
    else if (strcmp(topic, ESP32_REC_LIGHT_BRIGHTNESS_TOPIC) == 0)
    {
        status.light.brightness = (payload[0] - '0') * 100 + (payload[1] - '0') * 10 + (payload[2] - '0');
        calculate_light_color();
    }
}

void calculate_light_color()
{
    status.light.final_color[0] = (status.light.color[0] * status.light.brightness) >> 8;
    status.light.final_color[1] = (status.light.color[1] * status.light.brightness) >> 8;
    status.light.final_color[2] = (status.light.color[2] * status.light.brightness) >> 8;
    ESP_LOGI(TAG, "light color: %d %d %d", status.light.final_color[0], status.light.final_color[1], status.light.final_color[2]);
}

void mqtt_publish_dht11(void *pvParameter)
{
    while (1)
    {
        dht11_t dht11_data;
        if (xQueueReceive(Queue_dht11_mqtt, &dht11_data, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            char payload_temper[8];
            snprintf(payload_temper, sizeof(payload_temper), "%.1f", dht11_data.temp);
            esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_TEMPERATURE_TOPIC, payload_temper, strlen(payload_temper), 1, 1);
            char payload_humidity[8];
            snprintf(payload_humidity, sizeof(payload_humidity), "%d", dht11_data.humidity);
            esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_HUMIDITY_TOPIC, payload_humidity, strlen(payload_humidity), 1, 1);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void mqtt_publish_mq2(void *pvParameter)
{
    while (1)
    {
        float mq2_data;
        if (xQueueReceive(Queue_mq2_mqtt, &mq2_data, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            char payload_ppm[8];
            snprintf(payload_ppm, sizeof(payload_ppm), "%.1f", mq2_data);
            esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_SMOKE_TOPIC, payload_ppm, strlen(payload_ppm), 1, 1);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void mqtt_publish_volumn(void *pvParameter)
{
    while (1)
    {
        int volume_data;
        xQueueReceive(Queue_lvgl_volume_mqtt, &volume_data, portMAX_DELAY);
        char payload_volume[3];
        snprintf(payload_volume, sizeof(payload_volume), "%d", volume_data);
        esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_VOLUME_TOPIC, payload_volume, strlen(payload_volume), 1, 1);
        // vTaskDelay   (1000 / portTICK_PERIOD_MS);
    }
}

// void mqtt_pubilish_light_power(void *pvParameter)
// {
//     ESP_LOGE(TAG, "error");
//     while (1)
//     {
//         ESP_LOGE(TAG, "error");
//         char payload[4];
//         snprintf(payload, sizeof(payload), "%s", status.light.power ? "on" : "off");
//         ESP_LOGI(TAG, "mqtt publish light power: %s", payload);
//         esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_LIGHT_POWER_TOPIC, payload, strlen(payload), 1, 1);
//         // vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }
