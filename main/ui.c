#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "stdio.h"
#include "dht11.h"
#include "ui.h"
#include "esp_log.h"
#include "umqtt.h"
#include "mqtt_client.h"

static const char *TAG = "UI";

static lv_obj_t *slider_label;
static lv_obj_t *temp_val_label;
static lv_obj_t *humidity_val_label;
static lv_obj_t *AQI_val_label;
static lv_obj_t *weather_val_label;
static lv_obj_t *time_val_label;
static lv_obj_t *calendar;
static lv_obj_t *switch_ws2812;
static lv_obj_t *switch_cold;
static lv_obj_t *switch_wet;
static lv_obj_t *slider;

extern esp_mqtt_client_handle_t mqtt_handle;
#define ESP32_PUB_LIGHT_POWER_TOPIC "esp32/light_power"

LV_FONT_DECLARE(lvgl_font_siyuan_20);
LV_FONT_DECLARE(lvgl_font_siyuan_26);
LV_FONT_DECLARE(lvgl_font_siyuan_22);

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    status.volume = lv_slider_get_value(slider);
    xQueueSend(Queue_lvgl_volume_mqtt, &status.volume, 0);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", status.volume);
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void dht11_timer_cb(struct _lv_timer_t *t)
{
    dht11_t dht11;

    if (xQueueReceive(Queue_dht11_lvgl, &dht11, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        lv_label_set_text_fmt(temp_val_label, "%.1f", dht11.temp);
        lv_label_set_text_fmt(humidity_val_label, "%d%%", dht11.humidity);
    }
}

void switch_check_timer_cb(struct _lv_timer_t *t)
{
    status.light.power ? lv_obj_add_state(switch_ws2812, LV_STATE_CHECKED) : lv_obj_clear_state(switch_ws2812, LV_STATE_CHECKED);
    status.cold ? lv_obj_add_state(switch_cold, LV_STATE_CHECKED) : lv_obj_clear_state(switch_cold, LV_STATE_CHECKED);
    status.wet ? lv_obj_add_state(switch_wet, LV_STATE_CHECKED) : lv_obj_clear_state(switch_wet, LV_STATE_CHECKED);
}

void mqtt_airquality_timer_cb(struct _lv_timer_t *t)
{
    char air_quality[10];
    if (xQueueReceive(Queue_mqtt_airquality_lvgl, &air_quality, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        ESP_LOGI(TAG, "air_quality: %s", air_quality);
        lv_label_set_text_fmt(AQI_val_label, air_quality);
    }
}

void mqtt_weather_timer_cb(struct _lv_timer_t *t)
{
    char weather[10];
    if (xQueueReceive(Queue_mqtt_weather_lvgl, weather, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        ESP_LOGI(TAG, "weather: %s", weather);
        lv_label_set_text(weather_val_label, weather);
    }
}

void mqtt_time_timer_cb(struct _lv_timer_t *t)
{
    char time[10];
    if (xQueueReceive(Queue_mqtt_time_lvgl, time, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        ESP_LOGI(TAG, "time: %s", time);
        lv_label_set_text(time_val_label, time);
    }
}

void mqtt_date_timer_cb(struct _lv_timer_t *t)
{
    char date[15];
    if (xQueueReceive(Queue_mqtt_date_lvgl, date, pdMS_TO_TICKS(20)) == pdTRUE)
    {
        int year, month, day;
        year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 + (date[2] - '0') * 10 + (date[3] - '0');
        month = (date[5] - '0') * 10 + (date[6] - '0');
        day = (date[8] - '0') * 10 + (date[9] - '0');
        ESP_LOGI(TAG, "date: %d-%d-%d", year, month, day);
        lv_calendar_set_today_date(calendar, year, month, day);
        lv_calendar_set_showed_date(calendar, year, month);
    }
}

void mqtt_volumn_task(struct _lv_timer_t *t)
{
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", status.volume);
    lv_label_set_text(slider_label, buf);
    lv_slider_set_value(slider, status.volume, LV_ANIM_ON);
}

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_calendar_date_t date;
        if (lv_calendar_get_pressed_date(obj, &date))
        {
            LV_LOG_USER("Clicked date: %02d.%02d.%d", date.day, date.month, date.year);
        }
    }
}

static void switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    ESP_LOGI(TAG, "switch_event_handler");
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        ESP_LOGI(TAG, "value changed");
        LV_UNUSED(obj);
        if (obj == switch_ws2812)
        {
            status.light.power = lv_obj_has_state(obj, LV_STATE_CHECKED);
            char payload[4];
            snprintf(payload, sizeof(payload), "%s", status.light.power ? "on" : "off");
            esp_mqtt_client_publish(mqtt_handle, ESP32_PUB_LIGHT_POWER_TOPIC, payload, strlen(payload), 1, 0);
        }
        else if (obj == switch_cold)
        {
            status.cold = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }
        else if (obj == switch_wet)
        {
            status.wet = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }
        else
        {
            ESP_LOGI(TAG, "obj error");
        }
    }
}

void lv_right_ui(void)
{

    slider = lv_slider_create(lv_screen_active());
    lv_obj_set_pos(slider, 280, 270);
    lv_obj_set_size(slider, 150, 20);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_set_style_anim_duration(slider, 2000, 0);

    slider_label = lv_label_create(lv_screen_active());
    lv_label_set_text(slider_label, "0%");
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *volumn_label = lv_label_create(lv_screen_active());
    lv_label_set_text(volumn_label, "调节音量大小");
    lv_obj_set_width(volumn_label, 150);

    lv_obj_set_style_text_font(volumn_label, &lvgl_font_siyuan_20, 0);
    lv_obj_align_to(volumn_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 10, -50);

    lv_obj_t *temp_label = lv_label_create(lv_screen_active());
    lv_label_set_text(temp_label, "当前温度:");

    lv_obj_set_style_text_font(temp_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(temp_label, slider, LV_ALIGN_OUT_BOTTOM_MID, -40, -195);

    lv_obj_t *humidity_label = lv_label_create(lv_screen_active());
    lv_label_set_text(humidity_label, "当前湿度:");

    lv_obj_set_style_text_font(humidity_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(humidity_label, slider, LV_ALIGN_OUT_BOTTOM_MID, -40, -230);

    temp_val_label = lv_label_create(lv_screen_active());
    lv_label_set_text(temp_val_label, "");
    lv_obj_align_to(temp_val_label, temp_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    humidity_val_label = lv_label_create(lv_screen_active());
    lv_label_set_text(humidity_val_label, "");
    lv_obj_align_to(humidity_val_label, humidity_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_obj_t *symbol_label = lv_label_create(lv_screen_active());
    lv_label_set_text(symbol_label, "℃");
    lv_obj_set_style_text_font(symbol_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(symbol_label, temp_label, LV_ALIGN_OUT_RIGHT_MID, 50, 0);
}

void lv_left_ui(void)
{
    calendar = lv_calendar_create(lv_screen_active());
    lv_obj_set_size(calendar, 180, 180);
    lv_obj_set_pos(calendar, 20, 130);
    lv_obj_add_event_cb(calendar, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_text_font(calendar, &lv_font_montserrat_14, 0);
    lv_calendar_set_today_date(calendar, 2025, 04, 10);
    lv_calendar_set_showed_date(calendar, 2025, 04);

    lv_obj_t *label_weather = lv_label_create(lv_screen_active());
    lv_label_set_text(label_weather, "今日天气:");
    lv_obj_set_style_text_font(label_weather, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_weather, calendar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -265);

    lv_obj_t *label_AQI = lv_label_create(lv_screen_active());
    lv_label_set_text(label_AQI, "空气质量:");
    lv_obj_set_style_text_font(label_AQI, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_AQI, calendar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -235);

    lv_obj_t *label_time = lv_label_create(lv_screen_active());
    lv_label_set_text(label_time, "当前时间:");
    lv_obj_set_style_text_font(label_time, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_time, calendar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -205);

    weather_val_label = lv_label_create(lv_screen_active());
    lv_label_set_text(weather_val_label, "");
    lv_obj_set_style_text_font(weather_val_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(weather_val_label, label_weather, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    AQI_val_label = lv_label_create(lv_screen_active());
    lv_label_set_text(AQI_val_label, "");
    lv_obj_set_style_text_font(AQI_val_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(AQI_val_label, label_AQI, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    time_val_label = lv_label_create(lv_screen_active());
    lv_label_set_text(time_val_label, "");
    lv_obj_set_style_text_font(time_val_label, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(time_val_label, label_time, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    lv_calendar_header_dropdown_create(calendar);
}

void lv_title(void)
{
    lv_obj_t *label_t = lv_label_create(lv_screen_active());
    lv_label_set_text(label_t, "安颐智家");
    lv_obj_set_style_text_font(label_t, &lvgl_font_siyuan_26, 0);
    lv_obj_align(label_t, LV_ALIGN_CENTER, 0, -135);
}

void lv_switch_ws2812(void)
{

    switch_ws2812 = lv_switch_create(lv_screen_active());
    lv_obj_set_size(switch_ws2812, 65, 30);
    lv_obj_align(switch_ws2812, LV_ALIGN_CENTER, 50, -15);
    lv_obj_add_event_cb(switch_ws2812, switch_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(switch_ws2812, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *label_1 = lv_label_create(lv_screen_active());
    lv_label_set_text(label_1, "灯");
    lv_obj_set_style_text_font(label_1, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_1, switch_ws2812, LV_ALIGN_TOP_LEFT, 100, 5);
}

void lv_switch_cold(void)
{

    switch_cold = lv_switch_create(lv_screen_active());
    lv_obj_set_size(switch_cold, 65, 30);
    lv_obj_align(switch_cold, LV_ALIGN_CENTER, 50, 20);
    lv_obj_add_event_cb(switch_cold, switch_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(switch_cold, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *label_cold = lv_label_create(lv_screen_active());
    lv_label_set_text(label_cold, "空调");
    lv_obj_set_style_text_font(label_cold, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_cold, switch_cold, LV_ALIGN_TOP_LEFT, 100, 5);
}

void lv_switch_wet(void)
{

    switch_wet = lv_switch_create(lv_screen_active());
    lv_obj_set_size(switch_wet, 65, 30);
    lv_obj_align(switch_wet, LV_ALIGN_CENTER, 50, 55);
    lv_obj_add_event_cb(switch_wet, switch_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_add_flag(switch_wet, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t *label_wet = lv_label_create(lv_screen_active());
    lv_label_set_text(label_wet, "加湿器");
    lv_obj_set_style_text_font(label_wet, &lvgl_font_siyuan_22, 0);
    lv_obj_align_to(label_wet, switch_wet, LV_ALIGN_TOP_LEFT, 100, 5);
}

void app_main_display(void)
{
    lvgl_port_lock(0);
    lv_right_ui();
    lv_left_ui();
    lv_title();
    lv_switch_ws2812();
    lv_switch_cold();
    lv_switch_wet();
    lv_timer_create(dht11_timer_cb, 2000, NULL);
    lv_timer_create(mqtt_airquality_timer_cb, 2000, NULL);
    lv_timer_create(mqtt_weather_timer_cb, 2000, NULL);
    lv_timer_create(mqtt_time_timer_cb, 2000, NULL);
    lv_timer_create(mqtt_date_timer_cb, 2000, NULL);
    lv_timer_create(switch_check_timer_cb, 2000, NULL);
    lv_timer_create(mqtt_volumn_task, 2000, NULL);
    lvgl_port_unlock();
}
