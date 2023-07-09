/* Define interface of key manager(KM) */
#ifndef __SDP_KM_H__
#define __SDP_KM_H__

#include "sdp_utils.h"

#define SDP_KM_LEFT_GPIO    GPIO_NUM_5
#define SDP_KM_RIGHT_GPIO   GPIO_NUM_4

#define SDP_KM_UP_GPIO      GPIO_NUM_9
#define SDP_KM_DOWN_GPIO    GPIO_NUM_8

#define SDP_KM_MIDDLE_GPIO  GPIO_NUM_13


typedef struct _sdp_km_attr {
    int a;
} SDP_KM_ATTR;

SDP_HANDLE SDPI_KM_Create();

int8_t SDPI_KM_Destroy();

int8_t SDPI_KM_AddKey();

int8_t SDPI_KM_DeleteKey();

int8_t SDPI_KM_StartMonitor();

int8_t SDPI_KM_StopMonitor();

xQueueHandle SDPI_KM_GetEventQueue();
#endif