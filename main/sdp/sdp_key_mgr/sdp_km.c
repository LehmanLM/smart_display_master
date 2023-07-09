#include "sdp_km.h"
#include "driver/gpio.h"

#define SDP_KM_TAG "sdp_km"

#define GPIO_INPUT_PIN_SEL  ((1ULL<<SDP_KM_LEFT_GPIO) \
                            | (1ULL<<SDP_KM_RIGHT_GPIO) \
                            | (1ULL<<SDP_KM_MIDDLE_GPIO) \
                            | (1ULL<<SDP_KM_UP_GPIO) \
                            | (1ULL<<SDP_KM_DOWN_GPIO))
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void SDP_KM_MonitorKeysEventProc(void *data)
{
    uint32_t io_num;
    int gpio_level;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            gpio_level = gpio_get_level(io_num);
            ESP_LOGI(SDP_KM_TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_level);
            switch (io_num) {
                case SDP_KM_LEFT_GPIO:
                    ESP_LOGI(SDP_KM_TAG, "receive pushing button #LEFT# event");
                    break;
                case SDP_KM_RIGHT_GPIO:
                    ESP_LOGI(SDP_KM_TAG, "receive pushing button #RIGHT# event");
                    break;
                case SDP_KM_MIDDLE_GPIO:
                    ESP_LOGI(SDP_KM_TAG, "receive pushing button #MIDDLE# event");
                    break;
                default:
                    break;
            }
        }
        vTaskDelay(33/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

SDP_HANDLE SDPI_KM_Create()
{
    return NULL;
}

int8_t SDPI_KM_Destroy()
{
    return 0;
}

int8_t SDPI_KM_AddKey()
{
    return 0;
}

int8_t SDPI_KM_DeleteKey()
{
    return 0;
}

int8_t SDPI_KM_StartMonitor()
{
    gpio_config_t io_conf = {}; //zero-initialize the config structure.
    io_conf.intr_type = GPIO_INTR_POSEDGE; // 上升沿产生中断
    //io_conf.intr_type = GPIO_INTR_ANYEDGE; // 上升、下降沿都产生中断
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; //bit mask of the pins, use GPIO0 here
    io_conf.mode = GPIO_MODE_INPUT; //set as input mode
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1; //enable pull-up mode
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(2, sizeof(uint32_t));

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(SDP_KM_LEFT_GPIO, gpio_isr_handler, (void*) SDP_KM_LEFT_GPIO);
    gpio_isr_handler_add(SDP_KM_RIGHT_GPIO, gpio_isr_handler, (void*) SDP_KM_RIGHT_GPIO);
    gpio_isr_handler_add(SDP_KM_MIDDLE_GPIO, gpio_isr_handler, (void*) SDP_KM_MIDDLE_GPIO);
    gpio_isr_handler_add(SDP_KM_UP_GPIO, gpio_isr_handler, (void*) SDP_KM_UP_GPIO);
    gpio_isr_handler_add(SDP_KM_DOWN_GPIO, gpio_isr_handler, (void*) SDP_KM_DOWN_GPIO);

    /* Create thread to monitor keys events */
    //xTaskCreatePinnedToCore(SDP_KM_MonitorKeysEventProc, "sdp_dm_monitor_keys_event", 3 * 1024, NULL, 3, NULL, 1);
    return 0;
}

int8_t SDPI_KM_StopMonitor()
{
    return 0;
}

xQueueHandle SDPI_KM_GetEventQueue()
{
    return gpio_evt_queue;
}