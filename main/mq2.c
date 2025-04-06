#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "mq2.h"

float R0 = 0.0;

static adc_oneshot_unit_handle_t adc1_handle;

static void init_adc()
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, MQ2_CHANNEL, &config));
}

void get_MQ2_data(int *data)
{
    adc_oneshot_read(adc1_handle, MQ2_CHANNEL, data);
}

void calibrateR0()
{
    const int SAMPLE_COUNT = 50;
    float sumRs = 0.0;

    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {

        int sensorvalue;
        get_MQ2_data(&sensorvalue);
        float voltage = sensorvalue * (3.3 / 4095.0);
        float rs = ((3.3 - voltage) * RL) / voltage;

        sumRs += rs;
        vTaskDelay(100 / portTICK_PERIOD_MS);

        float avgRs = sumRs / SAMPLE_COUNT;            // 平均Rs
        R0 = avgRs / pow(CAL_PPM / 613.9, 1 / -2.074); // 计算R0
    }
}

float calculatePPM(float rs)
{
    if (R0 == 0.0)
    {
        return NAN;
    }

    float ppm = 613.9f * pow(rs / R0, -2.074f);
    return ppm;
}

void init_MQ2()
{
    init_adc();
    calibrateR0();
    if (!isnan(R0))
    {
        printf("校准的R0: %f Ohms\n", R0);
    }
    else
    {
        printf("校准失败\n");
    }
}
