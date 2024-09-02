#ifndef __APP_SERVER_H__
#define __APP_SERVER_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "lwip/sockets.h"
#include "cJSON.h"

#include "app_defines.h"
#include "app_events.h"
#include "app_vfs.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum request_network_type
  {
    NETWORK_TYPE_NONE = 0,
    NETWORK_TYPE_STA,
    NETWORK_TYPE_AP
  } request_network_type_t;

  typedef struct request_chunk_data_type
  {
    void *data;
    ssize_t size;
    ssize_t total;
    ssize_t processed;
  } request_chunk_data_t;

  esp_err_t init_server(app_config_t *app_config);

#ifdef __cplusplus
}
#endif

#endif // __APP_SERVER_H__
