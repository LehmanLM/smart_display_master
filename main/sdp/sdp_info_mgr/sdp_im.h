/* Define interface for information manager(IM) */
#ifndef __SDP_IM_H__
#define __SDP_IM_H__

/* wifi releated head file*/
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_netif_ip_addr.h>

/* Get internet information */
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/Queue.h"
#include "sdp_utils.h"

#define SDP_IM_MAX_STR_NUM  10
#define SDP_IM_MAX_STR_LEN  30

typedef struct sdp_im_connect_cfg{
    uint8_t ssid[32];      /* SSID of target AP. */
    uint8_t password[64];  /* Password of target AP. */
} SDP_IM_CONNECT_CFG;

typedef struct sdp_im_service_attr{
    char service_name[16];
    /* HTTP URL, the information on the URL is most important */
    /* it overrides the other fields below, if any */
    const char *url;
} SDP_IM_SERVICE_ATTR;

typedef struct sdp_im_respond{
    cJSON *root;
} SDP_IM_RESPOND;

/* SDPI_IM_Init - Initial information manager
 *
*/
int8_t SDPI_IM_Init();

int8_t SDPI_IM_Deinit();

int8_t SDPI_IM_GetAPList(wifi_ap_record_t *ap_list, uint16_t *ap_num);

SDP_HANDLE SDPI_IM_NewService(const SDP_IM_SERVICE_ATTR *_service_attr);

int8_t SDPI_IM_DestroyService(SDP_HANDLE service);

int8_t SDPI_IM_GetServiceRespond(SDP_HANDLE service, SDP_IM_RESPOND *respond);

int8_t SDPI_IM_PutServiceRespond(SDP_IM_RESPOND *_respond);
#endif