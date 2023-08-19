#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "freertos/semphr.h"

/* lvgl releated */
#include "lvgl.h"
#include "lvgl_helpers.h"
#include "ui_events.h"

#include "sdp_im.h"
#include "sdp_dm.h"
#include "ui.h"
#include "dragon_panda_128_128_01.c"
#include "dragon_panda_128_128_02.c"
#include "dragon_panda_128_128_03.c"
#include "rocket_128_128_01.c"
#include "bee_128_128.c"
#include "sdp_km.h"
#include "sdp_bm.h"

#define APP_LOG_TAG "sdp_app"

void led_task(void *argc)
{
    int print_times = 0;

    gpio_pad_select_gpio(GPIO_NUM_12);
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
    for(;;){
        gpio_set_level(GPIO_NUM_12, 0);
        gpio_set_level(GPIO_NUM_13, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_12, 1);
        gpio_set_level(GPIO_NUM_13, 0);
        vTaskDelay(500/portTICK_PERIOD_MS);
        ESP_LOGI(APP_LOG_TAG, "led task running[%d]", print_times);
        ++print_times;
    }
}

void test_im_task(void *argc)
{
    int8_t ret = 0;
    SDP_IM_SERVICE_ATTR service_attr = {
        .url = "https://api.seniverse.com/v3/weather/now.json?key=SpVO-lX5GreuiWDQo&location=shenzhen&language=zh-Hans&unit=c",
    };
    SDP_HANDLE sdp_ins = SDPI_IM_NewService(&service_attr);
    SDP_IM_RESPOND respond = {0};
    for(;;){
        ret = SDPI_IM_GetServiceRespond(sdp_ins, &respond);
        if (ret != ESP_OK) {
            ESP_LOGE(APP_LOG_TAG, "get service respond fail, try again");
            vTaskDelay(3000/portTICK_PERIOD_MS);
            continue;
        }
        cJSON *data = cJSON_GetObjectItem(respond.root, "results");
        cJSON *result = cJSON_GetArrayItem(data, 0);

        cJSON *location = cJSON_GetObjectItem(result, "location");
        cJSON *timezone = cJSON_GetObjectItem(location, "timezone");
        cJSON *name = cJSON_GetObjectItem(location, "name");
        ESP_LOGI(APP_LOG_TAG, "location: %s", name->valuestring);
        ESP_LOGI(APP_LOG_TAG, "timezone: %s", timezone->valuestring);
        
        cJSON *now = cJSON_GetObjectItem(result, "now");
        cJSON *temperature = cJSON_GetObjectItem(now, "temperature");
        ESP_LOGI(APP_LOG_TAG, "temperature: %s", temperature->valuestring);

        cJSON *last_update = cJSON_GetObjectItem(result, "last_update");
        ESP_LOGI(APP_LOG_TAG, "last_update: %s", last_update->valuestring);

        SDPI_IM_PutServiceRespond(&respond);
        vTaskDelay(3000/portTICK_PERIOD_MS);
    }
}

static int8_t app_create_gui()
{
#if 1
    ESP_LOGI(APP_LOG_TAG, "app create gui begin");
    ui_init();
    ESP_LOGI(APP_LOG_TAG, "app create gui end");
#endif
    return ESP_OK;
}

#define MAX_SRC_GIF_NUM 5
void test_dm_task(void *argc)
{
    SemaphoreHandle_t *dm_mutex = SDPI_DM_GetMutex();
    uint32_t io_num;
    int gpio_level;
    int src_idx = 0;
    void *src[MAX_SRC_GIF_NUM] = {&dragon_panda_128_128_01,
                                  &dragon_panda_128_128_02,
                                  &dragon_panda_128_128_03,
                                  &rocket_128_128_01,
                                  &bee_128_128};
    for(;;) {
        if(xQueueReceive(SDPI_KM_GetEventQueue(), &io_num, portMAX_DELAY)) {
            gpio_level = gpio_get_level(io_num);
            ESP_LOGI(APP_LOG_TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_level);
            switch (io_num) {
                case SDP_KM_LEFT_GPIO:
                case SDP_KM_UP_GPIO:
                    SDPI_BM_PlayClickSound();
                    src_idx++;
                    xSemaphoreTake(*dm_mutex, portMAX_DELAY);
                    lv_gif_set_src(img, src[src_idx % MAX_SRC_GIF_NUM]);
                    xSemaphoreGive(*dm_mutex);
                    ESP_LOGI(APP_LOG_TAG, "receive pushing button #LEFT# event");
                    break;
                case SDP_KM_RIGHT_GPIO:
                case SDP_KM_DOWN_GPIO:
                    SDPI_BM_PlayClickSound();
                    src_idx--;
                    if (src_idx < 0) {
                        src_idx = MAX_SRC_GIF_NUM - 1;
                    }
                    xSemaphoreTake(*dm_mutex, portMAX_DELAY);
                    lv_gif_set_src(img, src[src_idx % MAX_SRC_GIF_NUM]);
                    xSemaphoreGive(*dm_mutex);
                    ESP_LOGI(APP_LOG_TAG, "receive pushing button #RIGHT# event");
                    break;
                case SDP_KM_MIDDLE_GPIO:
                    SDPI_BM_PlayClickSound();
                    ESP_LOGI(APP_LOG_TAG, "receive pushing button #MIDDLE# event");
                    break;
                default:
                    break;
            }
            /* Clear key message queue */
            xQueueReset(SDPI_KM_GetEventQueue());
        }
        vTaskDelay(33/portTICK_PERIOD_MS);
    }
}
static inline void show_chip_info()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(APP_LOG_TAG, "This is %s chip with %d CPU core(s), WiFi%s%s ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    ESP_LOGI(APP_LOG_TAG, "silicon revision %d ", chip_info.revision);
    ESP_LOGI(APP_LOG_TAG, "%dMB %s flash", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(APP_LOG_TAG, "Minimum free heap size: %d bytes+1", esp_get_minimum_free_heap_size());
}

void app_main(void)
{
    SDPI_BM_Start();
    SDPI_BM_PlayBootSound();
    show_chip_info();
    SDPI_IM_Init();
    SDPI_DM_Init();
#if 0
    uint16_t ap_num = 8;
    wifi_ap_record_t ap_list[ap_num];
    SDPI_IM_GetAPList(ap_list, &ap_num);
#endif
    //xTaskCreatePinnedToCore(test_im_task, "test_im_task", 5 * 1024, NULL, 0, NULL, 0);
    SDP_DM_ATTR attr = {
        .cb = app_create_gui,
    };
    SDP_HANDLE dm_ins = SDPI_DM_NewDisplayIns(&attr);
    SDPI_DM_StartInstance(dm_ins);
    xTaskCreatePinnedToCore(test_dm_task, "test_dm_task", 3 * 1024, NULL, 0, NULL, 1);
    SDPI_KM_StartMonitor();
}