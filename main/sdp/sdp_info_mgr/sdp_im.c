#include "sdp_im.h"

#include "freertos/event_groups.h"
#include "esp_crt_bundle.h"

#define SDP_IM_TAG "sdp_im"

typedef enum wifi_state{
    SDP_IM_WIFI_START = 0,
    SDP_IM_WIFI_STOP,
    SDP_IM_WIFI_INVALID,
} WIFI_STATE;

WIFI_STATE wifi_state = SDP_IM_WIFI_STOP;
#define SDP_CHECK_IN(x) do { \
    if ((x) == NULL) { \
        ESP_LOGE(SDP_IM_TAG, "Input is NULL"); \
        return ESP_FAIL; \
    } \
} while(0)

static uint8_t sdp_im_service_num = 0;

typedef struct _sdp_im_service{
    uint8_t service_id;
    SDP_TYPE sdp_type;
    esp_http_client_handle_t client;
} SDP_IM_SERVICE;


/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
/* init nvs */
static inline void SDP_IM_InitNvs()
{
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/* wifi event handle*/
static void wifi_event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        wifi_event_sta_disconnected_t *disconnected;
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(SDP_IM_TAG, "wifi started");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(SDP_IM_TAG, "wifi connected");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                disconnected = (wifi_event_sta_disconnected_t *)event_data;
                ESP_LOGE(SDP_IM_TAG, "wifi disconnected, reason: %d", disconnected->reason);
                break;
            default:
                ESP_LOGW(SDP_IM_TAG, "wifi event, unknow event id[%d]", event_id);
                break;
        }
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(SDP_IM_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}
static esp_netif_t *g_esp_netif = NULL;
static void SDP_IM_WifiInit(const wifi_sta_config_t *sta_cfg)
{
    SDP_IM_InitNvs();
    esp_netif_init();
    esp_event_loop_create_default();
    g_esp_netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)sta_cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);
    esp_err_t err;
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(SDP_IM_TAG, "wifi start failed");
    }

    s_wifi_event_group = xEventGroupCreate();
   /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(SDP_IM_TAG, "connected to ap SSID:%s password:%s",
                 sta_cfg->ssid, sta_cfg->password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(SDP_IM_TAG, "Failed to connect to SSID:%s, password:%s",
                 sta_cfg->ssid, sta_cfg->password);
    } else {
        ESP_LOGE(SDP_IM_TAG, "UNEXPECTED EVENT");
    }
    wifi_state = SDP_IM_WIFI_START;
}

static void SDP_IM_WifiDeinit()
{
    esp_wifi_stop();
    esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP, wifi_event_handler);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler);
    esp_wifi_deinit();
    esp_netif_destroy_default_wifi(g_esp_netif);
    esp_event_loop_delete_default();
    esp_netif_deinit();
}

int8_t SDPI_IM_Init()
{
    wifi_sta_config_t sta_cfg = {
        .ssid = "Xiaomi_51F8",
        .password = "lm13539162475",
    };
    SDP_IM_WifiInit(&sta_cfg);
    return ESP_OK;
}

int8_t SDPI_IM_Deinit()
{
    SDP_IM_WifiDeinit();
    return ESP_OK;
}

int8_t SDPI_IM_SetConnectCfg(const SDP_IM_CONNECT_CFG *cfg)
{
    esp_wifi_stop();
    wifi_sta_config_t wifi_cfg = {0};
    esp_wifi_get_config(WIFI_IF_STA, (wifi_config_t *)&wifi_cfg);
    memcpy(wifi_cfg.ssid, cfg->ssid, 32);
    memcpy(wifi_cfg.password, cfg->password, 64);
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)&wifi_cfg);
    esp_wifi_start();
    return ESP_OK;
}

int8_t SDPI_IM_GetAPList(wifi_ap_record_t *ap_list, uint16_t *ap_num)
{
    if (ap_num == 0) {
        ESP_LOGE(SDP_IM_TAG, "Desire number of ap_list can't be 0");
        return ESP_FAIL;
    }
    /* if wifi connected, scanning ap will fail */
    if (wifi_state == SDP_IM_WIFI_START) {
        esp_wifi_stop();
        wifi_state = SDP_IM_WIFI_STOP;
        ESP_LOGI(SDP_IM_TAG, "Stop wifi for scanning AP");
    }
    uint8_t rssi_level = 0;
    uint16_t num = *ap_num;
    ESP_LOGI(SDP_IM_TAG, "begin to scan ap");
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_num(&num);
    esp_wifi_scan_get_ap_records(&num, ap_list);
    ESP_LOGI(SDP_IM_TAG, "Actural scan ap num is %d", num);
    if(*ap_num > num){
        *ap_num = num;
    }
    for(int i = 0; i < *ap_num; ++i){
        rssi_level = 0;
        switch(ap_list[i].rssi){
            case -100 ... -88:
                rssi_level = 1;
                break;
            case -87 ... -78:
                rssi_level = 2;
                break;
            case -77 ... -66:
                rssi_level = 3;
                break;
            case -65 ... -55:
                rssi_level = 4;
                break;
            default:
                if(ap_list[i].rssi < -100){
                    rssi_level = 0;
                }else{
                    rssi_level = 5;
                }
                break;
        }
        ESP_LOGI(SDP_IM_TAG, "--Num[%d]--Wifi[%s]-- ", i + 1, ap_list[i].ssid);
        ESP_LOGI(SDP_IM_TAG, "Signal Strength[%d] ", rssi_level);
        ESP_LOGI(SDP_IM_TAG, "Wifi AuthMode[%d]\n\n", ap_list[i].authmode);
    }
    return ESP_OK;
}

SDP_HANDLE SDPI_IM_NewService(const SDP_IM_SERVICE_ATTR *_service_attr)
{
    if (_service_attr == NULL){
        ESP_LOGE(SDP_IM_TAG, "invalid input NULL");
        return NULL;
    }
    SDP_IM_SERVICE *service = (SDP_IM_SERVICE *)malloc(sizeof(SDP_IM_SERVICE));
    service->service_id = sdp_im_service_num++;
    service->sdp_type = SDP_IM;

    esp_http_client_config_t http_cfg = {0};
    http_cfg.url = _service_attr->url;
    http_cfg.transport_type = HTTP_TRANSPORT_OVER_SSL;
    http_cfg.crt_bundle_attach = esp_crt_bundle_attach;
    service->client = esp_http_client_init(&http_cfg);
    esp_http_client_set_method(service->client, HTTP_METHOD_GET);
    return (SDP_HANDLE)service;
}

int8_t SDPI_IM_DestroyService(SDP_HANDLE _service)
{
    SDP_IM_SERVICE *service = (SDP_IM_SERVICE *)_service;
    if (service->sdp_type != SDP_IM) {
        ESP_LOGE(SDP_IM_TAG, "wrong sdp type[%d]", service->sdp_type);
        return ESP_FAIL;
    }
    esp_http_client_cleanup(service->client);
    free(service);
    return ESP_OK;
}

#define HTTP_OUTPUT_BUFFER_SIZE 2048
int8_t SDPI_IM_GetServiceRespond(SDP_HANDLE ins, SDP_IM_RESPOND *_respond)
{
    SDP_IM_SERVICE *service = (SDP_IM_SERVICE *)ins;
    if (service->sdp_type != SDP_IM) {
        ESP_LOGE(SDP_IM_TAG, "wrong sdp type[%d]", service->sdp_type);
        return ESP_FAIL;
    }
    if (_respond == NULL) {
        ESP_LOGE(SDP_IM_TAG, "invalid input respond: NULL");
        return ESP_FAIL;
    }
    esp_err_t err = esp_http_client_open(service->client, 0);
    if(err != ESP_OK){
        ESP_LOGE(SDP_IM_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else{
        char output_buffer[HTTP_OUTPUT_BUFFER_SIZE] = {0};
        int content_length = 0;
        content_length = esp_http_client_fetch_headers(service->client);
        if(content_length < 0){
            ESP_LOGE(SDP_IM_TAG, "HTTP client fetch header failed");
        }
        else{
            int data_read = esp_http_client_read_response(service->client, output_buffer, HTTP_OUTPUT_BUFFER_SIZE);
            if(data_read >= 0){
                 _respond->root = cJSON_Parse(output_buffer);
            }else{
                ESP_LOGE(SDP_IM_TAG, "Failed to read response");
            }
        }
    }
    esp_http_client_close(service->client);
    return ESP_OK;
}

int8_t SDPI_IM_PutServiceRespond(SDP_IM_RESPOND *_respond)
{
    if (_respond == NULL) {
        ESP_LOGE(SDP_IM_TAG, "invalid input respond: NULL");
        return ESP_FAIL;
    }
    if (_respond->root != NULL) {
        cJSON_Delete(_respond->root);
    }
    return ESP_OK;
}

