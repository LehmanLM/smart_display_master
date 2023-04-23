#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "sdp/sdp_info_mgr/sdp_im.h"

#define APP_LOG_TAG "sdp"

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

#define TIME_QUEUE_LENGTH 2
#define TIME_QUEUE_SIZE 100
void app_main(void)
{
    show_chip_info();
    QueueHandle_t _time_queue;
    _time_queue = xQueueCreate(TIME_QUEUE_LENGTH, TIME_QUEUE_SIZE);
    if(_time_queue == NULL){
        ESP_LOGE(APP_LOG_TAG, "Time queue creating failed");
        return;
    }
    SDPI_IM_Init();
    //uint16_t ap_num = 8;
    //wifi_ap_record_t ap_list[ap_num];
    //SDPI_IM_GetAPList(ap_list, &ap_num);
    xTaskCreatePinnedToCore(test_im_task, "test_im_task", 5 * 1024, NULL, 2, NULL, 0);
#if 0
    wifi_init();
    xTaskCreatePinnedToCore(time_info_task, "time_info_task", 10 * 1024, (void *)_time_queue, 2, &wifi_scan_handle, 0);
    xTaskCreatePinnedToCore(gui_task, "guiTask", 10 * 1024, (void *)_time_queue, 2, NULL, 1);
    xTaskCreatePinnedToCore(led_task, "led_task", 512, NULL, 2, NULL, 1);
#endif
}