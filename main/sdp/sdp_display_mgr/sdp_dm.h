/* Define interface of display manager(DM) */
#ifndef __SDP_DM_H__
#define __SDP_DM_H__

#include "sdp_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef int8_t (*SDP_DM_SetUpGuiFunc)();

typedef struct sdp_dm_attr{
    SDP_DM_SetUpGuiFunc cb;
} SDP_DM_ATTR;

int8_t SDPI_DM_Init();

int8_t SDPI_DM_Deinit();

SDP_HANDLE SDPI_DM_NewDisplayIns(SDP_DM_ATTR *attr);

int8_t SDPI_DM_DestroyDisplayIns(SDP_HANDLE display_ins);

int8_t SDPI_DM_StartInstance(SDP_HANDLE display_ins);

SemaphoreHandle_t *SDPI_DM_GetMutex();

#endif