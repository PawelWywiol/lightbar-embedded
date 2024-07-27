#ifndef __APP_NVS_H__
#define __APP_NVS_H__

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_defines.h"

esp_err_t nvs_init(void);
esp_err_t nvs_read_data(const char *key, void *data, size_t length);
esp_err_t nvs_write_data(const char *key, void *data, size_t length);

#endif // __APP_NVS_H__
