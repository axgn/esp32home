简要说明：这是一个运行在 ESP32 (IDF 5.4.0) 的智能家居演示工程，集成了 LVGL 显示、触摸、DHT11 温湿度、MQ2 烟雾检测、WS2812 灯带和 MQTT 通信，同时使用freertos去处理多个task。

快速导航：
- 源代码入口：[main/main.c](main/main.c)（包含任务创建与主循环）
- 主构建脚本：[CMakeLists.txt](CMakeLists.txt) 与 [main/CMakeLists.txt](main/CMakeLists.txt)
- 分区表：[partitions.csv](partitions.csv)
- 项目配置：[sdkconfig](sdkconfig) / 旧配置备份 [sdkconfig.old](sdkconfig.old)

主要功能与位置：
- 温湿度传感器：初始化与读取接口 [`DHT11_Init`](main/dht11.h), [`DHT11_StartGet`](main/dht11.h)（实现见 [main/dht11.c](main/dht11.c)）
- 烟雾传感器（MQ2）：初始化与数据处理 [`init_MQ2`](main/mq2.h), [`get_MQ2_data`](main/mq2.h), [`calculatePPM`](main/mq2.h)（实现见 [main/mq2.c](main/mq2.c)）
- MQTT：连接与发布函数 [`init_mqtt`](main/mqtt.h), 任务例如 [`mqtt_publish_volumn`](main/mqtt.h)（实现见 [main/mqtt.c](main/mqtt.c)）
- LVGL UI：界面构建与显示入口 [`app_main_display`](main/ui.c)（实现见 [main/ui.c](main/ui.c)）
- LED 灯带（WS2812）：初始化 [`init_ws2812`](main/main.c)（实现见 [main/main.c](main/main.c)）
- 任务/队列初始化：[`init_queue`](main/main.c)（见 [main/main.c](main/main.c)）

第三方组件：
- FT5x06 触摸驱动组件：实现与示例见 [components/espressif__esp_lcd_touch_ft5x06/esp_lcd_touch_ft5x06.c](components/espressif__esp_lcd_touch_ft5x06/esp_lcd_touch_ft5x06.c) 与说明 [components/espressif__esp_lcd_touch_ft5x06/README.md](components/espressif__esp_lcd_touch_ft5x06/README.md)

快速开始（在已安装 ESP-IDF 的环境下）：
1. 在项目根目录初始化 IDF 环境（参照本机 ESP-IDF 文档）。
2. 配置（可选）：检查 [sdkconfig](sdkconfig) 或运行 `idf.py menuconfig` 生成。
3. 构建：
   ```sh
   idf.py build
   ```
4. 烧录并监控：
   ```sh
   idf.py -p <PORT> flash monitor
   ```

常见入口与帮助：
- 查看主任务启动与任务创建：[main/main.c](main/main.c)
- UI 入口与定时器任务：[main/ui.c](main/ui.c)
- 传感器驱动和解析：[main/dht11.c](main/dht11.c), [main/mq2.c](main/mq2.c)
- MQTT 实现：[main/mqtt.c](main/mqtt.c), 对应声明 [main/mqtt.h](main/mqtt.h)
- 组件依赖说明：[main/idf_component.yml](main/idf_component.yml)

许可
- 项目中使用的第三方组件（例如 FT5x06 驱动）包含其 license 文件，参见 [components/espressif__esp_lcd_touch_ft5x06/license.txt](components/espressif__esp_lcd_touch_ft5x06/license.txt)。
