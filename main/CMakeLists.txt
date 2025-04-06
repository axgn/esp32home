idf_component_register(SRCS "wifi.c" "mqtt.c" "mq2.c" "dht11.c" "ui.c" "lvgl_init.c" "main.c" "font/lvgl_font_siyuan_20.c" "font/lvgl_font_siyuan_22.c" "font/lvgl_font_siyuan_26.c"
                    INCLUDE_DIRS ".")

# lvgl_port_create_c_image("images/esp_logo.png" "images/" "ARGB8888" "NONE")
# lvgl_port_add_images(${COMPONENT_LIB} "images/")
