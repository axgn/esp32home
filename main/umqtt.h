#ifndef UMQTT_H
#define UMQTT_H

#include "freertos/queue.h"
#include "ui.h"

extern QueueHandle_t Queue_dht11_lvgl;
extern QueueHandle_t Queue_mqtt_weather_lvgl;
extern QueueHandle_t Queue_mqtt_weather_lvgl;
extern QueueHandle_t Queue_mqtt_time_lvgl;
extern QueueHandle_t Queue_mqtt_date_lvgl;
extern QueueHandle_t Queue_mqtt_airquality_lvgl;
extern QueueHandle_t Queue_dht11_mqtt;
extern QueueHandle_t Queue_mq2_mqtt;
// extern QueueHandle_t Queue_mqtt_switch_light_lvgl;
// extern QueueHandle_t Queue_mqtt_switch_wet_lvgl;
// extern QueueHandle_t Queue_mqtt_switch_cold_lvgl;
extern QueueHandle_t Queue_lvgl_volume_mqtt;
extern status_t status;

#endif
