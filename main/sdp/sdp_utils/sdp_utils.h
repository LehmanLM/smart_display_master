#ifndef _SDP_UTILS_H_
#define _SDP_UTILS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define SDP_HANDLE void*

typedef enum sdp_type{
    SDP_IM = 0,
    SDP_INVALID,
} SDP_TYPE;
#endif