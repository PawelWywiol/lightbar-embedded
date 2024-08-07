#ifndef __APP_SERVER_H__
#define __APP_SERVER_H__

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include "app_defines.h"
#include "app_vfs.h"

esp_err_t init_server(void);

#endif // __APP_SERVER_H__
