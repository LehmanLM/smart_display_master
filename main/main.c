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
#include "freertos/semphr.h"

/* lvgl releated */
#include "lvgl.h"
#include "lvgl_helpers.h"

#include "sdp_im.h"
#include "sdp_dm.h"

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
    /* Get the current screen  */
    lv_obj_t * src = lv_disp_get_scr_act(NULL);
    /*Create a Label on the currently active screen*/
    //lv_obj_t * label1 =  lv_label_create(scr);
    /*Modify the Label's text*/
    //lv_label_set_text(label1, "Hello\nworld");
    lv_obj_t *bt0 = lv_btn_create(src);
    lv_obj_set_size(bt0, 30, 20);
    lv_obj_set_pos(bt0, 0, 0);
    lv_obj_t * label0 = lv_label_create(bt0);
    lv_label_set_text(label0, "btn1");
    lv_obj_center(label0);

    lv_obj_t *bt1 = lv_btn_create(src);
    lv_obj_set_size(bt1, 30, 20);
    lv_obj_set_pos(bt1, 98, 0);
    lv_obj_t * label1 = lv_label_create(bt1);
    lv_label_set_text(label1, "btn1");
    lv_obj_center(label1);

    lv_obj_t *bt2 = lv_btn_create(src);
    lv_obj_set_size(bt2, 30, 20);
    lv_obj_set_pos(bt2, 0, 108);
    lv_obj_t * label2 = lv_label_create(bt2);
    lv_label_set_text(label2, "btn1");
    lv_obj_center(label2);

    lv_obj_t *bt3 = lv_btn_create(src);
    lv_obj_set_size(bt3, 30, 20);
    lv_obj_set_pos(bt3, 98, 108);
    lv_obj_t * label3 = lv_label_create(bt3);
    lv_label_set_text(label3, "btn1");
    lv_obj_center(label3);
    /* Align the Label to the center
     * NULL means align on parent (which is the screen now)
     * 0, 0 at the end means an x, y offset after alignment*/
    //lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGI(APP_LOG_TAG, "app create gui end");
#endif
    return ESP_OK;
}

void test_dm_task(void *argc)
{
    //SemaphoreHandle_t *dm_mutex = SDPI_DM_GetMutex();
    for (;;){
        ESP_LOGI(APP_LOG_TAG, "test_dm_task");
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

void app_main(void)
{
    show_chip_info();
    SDPI_IM_Init();
    SDPI_DM_Init();
#if 0
    uint16_t ap_num = 8;
    wifi_ap_record_t ap_list[ap_num];
    SDPI_IM_GetAPList(ap_list, &ap_num);
#endif
    xTaskCreatePinnedToCore(test_im_task, "test_im_task", 5 * 1024, NULL, 0, NULL, 0);

    SDP_DM_ATTR attr = {
        .cb = app_create_gui,
    };
    SDP_HANDLE dm_ins = SDPI_DM_NewDisplayIns(&attr);
    SDPI_DM_StartInstance(dm_ins);
    xTaskCreatePinnedToCore(test_dm_task, "test_dm_task", 4 * 1024, NULL, 0, NULL, 1);
}