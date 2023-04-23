#include "sdp_dm.h"


#define LV_TICK_PERIOD_MS 1
static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void update_text(lv_obj_t *obj, char *str)
{
    lv_label_set_text(obj, str);
}
// lvgl draw compoments
static void lvgl_draw_init(lv_obj_t *lb_obj)
{
    lv_label_set_long_mode(lb_obj, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    lv_label_set_recolor(lb_obj, true);                 /*Enable re-coloring by commands in the text*/
    lv_label_set_text(lb_obj, "begin");
    lv_obj_set_width(lb_obj, 90); /*Set smaller width to make the lines wrap*/
    lv_obj_set_style_text_align(lb_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lb_obj, LV_ALIGN_CENTER, 0, 0);
#if 0
    lv_obj_t *label2 = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR); /*Circular scroll*/
    lv_obj_set_width(label2, 128);
    lv_label_set_text(label2, "It is a circularly scrolling text. ");
    //lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_pos(label2, 0, 30);
#endif
}
/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;
static void gui_task(void *argc)
{
    QueueHandle_t time_queue = (QueueHandle_t )argc;
    xGuiSemaphore = xSemaphoreCreateMutex();
    char receive_data[100];
    lv_init();
    lvgl_driver_init();

    ESP_LOGI(TAG, "DISP_BUF_SIZE[%d]\n", DISP_BUF_SIZE);
    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 != NULL);
    static lv_color_t *buf2 = NULL;

    static lv_disp_draw_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.physical_hor_res = -1;
    disp_drv.physical_ver_res = -1;
    lv_disp_drv_register(&disp_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
                                                            .callback = &lv_tick_task,
                                                            .name = "periodic_gui"
                                                        };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    lv_obj_t *label1 = lv_label_create(lv_scr_act());
    lvgl_draw_init(label1);
    BaseType_t status;
    for(;;){
        memset(receive_data, 0, sizeof(receive_data));
        status = xQueueReceive(time_queue, receive_data, portMAX_DELAY);
        if(status == pdPASS){
            update_text(label1, receive_data);
        }
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
    free(buf1);
    vTaskDelete(NULL);
}
