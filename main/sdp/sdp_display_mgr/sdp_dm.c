#include "sdp_dm.h"

/* lvgl releated */
#include "lvgl.h"
#include "lvgl_helpers.h"

//------------------------------------------------------------
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

/* Littlevgl specific */
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
//-----------------------------------------------------------
#define SDP_DM_TAG "sdp_dm"

typedef struct _sdp_dm_ins{
    uint8_t ins_id;
    SDP_DM_SetUpGuiFunc cb;
} SDP_DM_INS;

typedef enum _sdp_dm_status{
    SDP_DM_STATUS_INITED = 0,
    SDP_DM_STATUS_STARTED,
    SDP_DM_STATUS_STOPED,
    SDP_DM_STATUS_INVALID,
} SDP_DM_STATUS;

static SDP_DM_STATUS sdp_dm_status = SDP_DM_STATUS_INVALID;
static SemaphoreHandle_t sdp_dm_mutex;
static lv_color_t *buf1 = NULL;

#define LV_TICK_PERIOD_MS 1
static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
int8_t SDPI_DM_Init()
{
    sdp_dm_mutex = xSemaphoreCreateMutex();
    sdp_dm_status = SDP_DM_STATUS_INITED;
    return ESP_OK;
}

int8_t SDPI_DM_Deinit()
{
    if (buf1 != NULL) {
        heap_caps_free(buf1);
    }
    return ESP_OK;
}

static uint8_t sdp_dm_ins_num = 0;
SDP_HANDLE SDPI_DM_NewDisplayIns(SDP_DM_ATTR *dm_attr)
{
    if (NULL == dm_attr) {
        ESP_LOGE(SDP_DM_TAG, "ins attr is NULL");
        return NULL;
    }
    SDP_DM_INS *ins = (SDP_DM_INS *)malloc(sizeof(SDP_DM_INS));
    ins->ins_id = (sdp_dm_ins_num++);
    ins->cb = dm_attr->cb;
    ESP_LOGI(SDP_DM_TAG, "Create display ins[%d]", ins->ins_id);
    return (SDP_HANDLE)ins;
}

int8_t SDPI_DM_DestroyDisplayIns(SDP_HANDLE display_ins)
{
    if (display_ins == NULL) {
        ESP_LOGE(SDP_DM_TAG, "display ins is NULL");
        return ESP_LOG_ERROR;
    }
    SDP_DM_INS *ins = (SDP_DM_INS *)display_ins;
    free(ins);
    return ESP_OK;
}

/* Creates a recursive semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
static void SDP_DM_ReFleshScreenProc(void *data)
{
    ESP_LOGI(SDP_DM_TAG, "flesh screen begin");
    lv_init();
    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();
    lv_color_t* buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t* buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif
    static lv_disp_draw_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.hor_res = 128;
    disp_drv.ver_res = 128;
#if 1
    disp_drv.rotated = 1;
#endif
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = 
    {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    SDP_DM_INS *dm_ins = (SDP_DM_INS *) data;
    dm_ins->cb();

    for(;;){
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(sdp_dm_mutex, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(sdp_dm_mutex);
        }
        vTaskDelay(33/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

int8_t SDPI_DM_StartInstance(SDP_HANDLE display_ins)
{
    if (sdp_dm_status == SDP_DM_STATUS_INVALID) {
        ESP_LOGE(SDP_DM_TAG, "SDP DM module not init");
        return ESP_LOG_ERROR;
    }
    if (display_ins == NULL) {
        ESP_LOGE(SDP_DM_TAG, "display ins is NULL");
        return ESP_LOG_ERROR;
    }
    /* Create thread to update GUI */
    xTaskCreatePinnedToCore(SDP_DM_ReFleshScreenProc, "sdp_dm_flesh_screen", 5 * 1024, display_ins, 0, NULL, 1);
    sdp_dm_status = SDP_DM_STATUS_STARTED;
    return ESP_OK;
}

SemaphoreHandle_t *SDPI_DM_GetMutex()
{
    return &sdp_dm_mutex;
}
