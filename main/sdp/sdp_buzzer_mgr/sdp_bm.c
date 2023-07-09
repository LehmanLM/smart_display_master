#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "sdp_bm.h"
#include "esp_log.h"

#define c 261
#define d 294
#define e 329
#define f 349
#define g 391
#define gS 415
#define a 440
#define aS 455
#define b 466
#define cH 523
#define cSH 554
#define dH 587
#define dSH 622
#define eH 659
#define fH 698
#define fSH 740
#define gH 784
#define gSH 830
#define aH 880

#define SDP_BUZZER_GPIO_OUTPUT GPIO_NUM_19
//#define GPIO_OUTPUT_SPEED LEDC_LOW_SPEED_MODE // back too old git commit :-(
#define GPIO_OUTPUT_SPEED LEDC_LOW_SPEED_MODE

#define SDP_BM_TAG "sdp_bm"

// do（哆）、re（来）、mi（咪）、fa（发）、sol（唆）、la（拉）、si（西）
static int doremi[] = {262, 294, 330, 370, 392, 440, 494, 523,587,659,698,784,880,988} ;

int8_t SDPI_BM_Start()
{
    esp_err_t ret;
    ledc_timer_config_t timer_conf = {0};
    timer_conf.speed_mode = GPIO_OUTPUT_SPEED;
    timer_conf.bit_num    = LEDC_TIMER_13_BIT;
    timer_conf.timer_num  = LEDC_TIMER_1;
    timer_conf.freq_hz    = 5000;
    timer_conf.clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(SDP_BM_TAG, "ledc timer config fail, ret[%d]", ret);
        return ESP_FAIL;
    }
    ledc_channel_config_t ledc_conf = {0};
    ledc_conf.gpio_num   = SDP_BUZZER_GPIO_OUTPUT;
    ledc_conf.speed_mode = GPIO_OUTPUT_SPEED;
    ledc_conf.channel    = LEDC_CHANNEL_1;
    ledc_conf.intr_type  = LEDC_INTR_DISABLE;
    ledc_conf.timer_sel  = LEDC_TIMER_1;
    ledc_conf.duty       = 0x0; // 50%=0x3FFF, 100%=0x7FFF for 15 Bit
                                // 50%=0x01FF, 100%=0x03FF for 10 Bit
    ret = ledc_channel_config(&ledc_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(SDP_BM_TAG, "ledc channle config fail, ret[%d]", ret);
        return ESP_FAIL;
    }
    ledc_set_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_1, 254); 
    ledc_update_duty(GPIO_OUTPUT_SPEED, LEDC_CHANNEL_1);
    ledc_timer_pause(GPIO_OUTPUT_SPEED, LEDC_TIMER_1);

    return ESP_OK;
}

int8_t SDPI_BM_PlayClickSound()
{
    ESP_LOGI(SDP_BM_TAG, "play begin");
    int click_sound[] = {262, 294, 330, 370, 392, 440, 494, 523,587,659,698,784,880,988} ;
    ledc_timer_resume(GPIO_OUTPUT_SPEED, LEDC_TIMER_1);
    ledc_set_freq(GPIO_OUTPUT_SPEED, LEDC_TIMER_1, click_sound[6]);
    vTaskDelay(120/portTICK_PERIOD_MS);
    ledc_set_freq(GPIO_OUTPUT_SPEED, LEDC_TIMER_1, click_sound[13]);
    vTaskDelay(40/portTICK_PERIOD_MS);
    ledc_timer_pause(GPIO_OUTPUT_SPEED, LEDC_TIMER_1);
    ESP_LOGI(SDP_BM_TAG, "play end");
    return ESP_OK;
}

int8_t SDPI_BM_PlayBootSound()
{
    ESP_LOGI(SDP_BM_TAG, "play begin");
    int click_sound[] = {262, 294, 330, 370, 392, 440, 494, 523,587,659,698,784,880,988} ;
    ledc_timer_resume(GPIO_OUTPUT_SPEED, LEDC_TIMER_1);
    for (int i = 0; i < 14; i++) {
        ledc_set_freq(GPIO_OUTPUT_SPEED, LEDC_TIMER_1, click_sound[i]);
        vTaskDelay(150/portTICK_PERIOD_MS);
    }
    ledc_timer_pause(GPIO_OUTPUT_SPEED, LEDC_TIMER_1);
    ESP_LOGI(SDP_BM_TAG, "play end");
    return ESP_OK;
}