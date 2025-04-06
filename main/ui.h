#ifndef _UI_H_
#define _UI_H_
#include "stdbool.h"

typedef struct light_s
{
    bool power;
    uint8_t mode;
    uint8_t brightness;
    uint8_t color[3];
    uint8_t final_color[3];
} light_t;

typedef struct status_s
{
    int volume;
    light_t light;
    bool wet;
    bool cold;
} status_t;

void app_main_display(void);

#endif
