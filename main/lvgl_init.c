
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_ft5x06.h"
#include "driver/i2c.h"
#include "esp_lcd_st7796.h"

static const char *TAG = "LVGL_INIT";

#define ST7796_H_RES (480)
#define ST7796_V_RES (320)

#define ST7796_SPI_NUM (SPI2_HOST)
#define ST7796_PIXEL_CLK_HZ (60 * 1000 * 1000)
#define ST7796_CMD_BITS (8)
#define ST7796_PARAM_BITS (8)
#define ST7796_COLOR_SPACE (ESP_LCD_COLOR_SPACE_BGR)
#define ST7796_BITS_PER_PIXEL (16)
#define ST7796_DRAW_BUFF_DOUBLE (1)
#define ST7796_DRAW_BUFF_HEIGHT (50)
#define ST7796_BL_ON_LEVEL (1)

#define ST7796_GPIO_SCLK (GPIO_NUM_26)
#define ST7796_GPIO_MOSI (GPIO_NUM_15)
#define ST7796_GPIO_RST (GPIO_NUM_12)
#define ST7796_GPIO_DC (GPIO_NUM_5)
#define ST7796_GPIO_CS (GPIO_NUM_13)
#define ST7796_GPIO_BL (GPIO_NUM_27)

#define FT6336_I2C_NUM (0)
#define FT6336_I2C_CLK_HZ (200000)

#define FT6336_RESET_GPIO (GPIO_NUM_21)
#define FT6336_I2C_SCL (GPIO_NUM_22)
#define FT6336_I2C_SDA (GPIO_NUM_23)
#define FT6336_GPIO_INT (GPIO_NUM_19)

static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

static lv_display_t *lvgl_disp = NULL;
static lv_indev_t *lvgl_touch_indev = NULL;

esp_err_t app_lcd_init(void)
{

    esp_err_t ret = ESP_OK;

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << ST7796_GPIO_BL};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = ST7796_GPIO_SCLK,
        .mosi_io_num = ST7796_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = ST7796_H_RES * ST7796_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(ST7796_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = ST7796_GPIO_DC,
        .cs_gpio_num = ST7796_GPIO_CS,
        .pclk_hz = ST7796_PIXEL_CLK_HZ,
        .lcd_cmd_bits = ST7796_CMD_BITS,
        .lcd_param_bits = ST7796_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)ST7796_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = ST7796_GPIO_RST,
        .color_space = ST7796_COLOR_SPACE,
        .bits_per_pixel = ST7796_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7796(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    ESP_ERROR_CHECK(gpio_set_level(ST7796_GPIO_BL, ST7796_BL_ON_LEVEL));

    return ret;

err:
    if (lcd_panel)
    {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io)
    {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(ST7796_SPI_NUM);
    return ret;
}

esp_err_t app_touch_init(void)
{

    ESP_LOGI(TAG, "Touch I2C rest successfully");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = FT6336_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = FT6336_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = FT6336_I2C_CLK_HZ};
    ESP_RETURN_ON_ERROR(i2c_param_config(FT6336_I2C_NUM, &i2c_conf), TAG, "I2C configuration failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(FT6336_I2C_NUM, i2c_conf.mode, 0, 0, 0), TAG, "I2C initialization failed");
    ESP_LOGI(TAG, "I2C initialized successfully");

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = ST7796_H_RES,
        .y_max = ST7796_V_RES,
        .rst_gpio_num = FT6336_RESET_GPIO,
        .int_gpio_num = FT6336_GPIO_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = true,
            .mirror_x = false,
            .mirror_y = false,
        },
    };
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)FT6336_I2C_NUM, &tp_io_config, &tp_io_handle), TAG, "");
    return esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &touch_handle);
}

esp_err_t app_lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096 * 2,
        .task_affinity = 0,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5};
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = ST7796_H_RES * ST7796_DRAW_BUFF_HEIGHT,
        .double_buffer = ST7796_DRAW_BUFF_DOUBLE,
        .hres = ST7796_H_RES,
        .vres = ST7796_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = true,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = lvgl_disp,
        .handle = touch_handle,
    };
    lvgl_touch_indev = lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}
