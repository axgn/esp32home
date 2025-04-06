#ifndef _MQTT_H_
#define _MQTT_H_

void init_mqtt();

void mqtt_publish_dht11(void *pvParameter);

void mqtt_publish_mq2(void *pvParameter);

void mqtt_publish_volumn(void *pvParameter);

void mqtt_pubilish_light_power(void *pvParameter);

#endif
