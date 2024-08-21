#ifndef __APP_SERVER_H__
#define __APP_SERVER_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "app_defines.h"
#include "app_vfs.h"

esp_err_t set_api_response(httpd_req_t *req, char *message);
esp_err_t init_server(app_config_t *app_config);

#endif // __APP_SERVER_H__
