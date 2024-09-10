#ifndef __APP_UTILS_H__
#define __APP_UTILS_H__

#include "esp_err.h"
#include "esp_log.h"
#include <string.h>

#include "app_defines.h"
#include "app_nvs.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void uid(char *uid, size_t length);

  void reset_wifi_credentials(wifi_credentials_t *wifi_credentials);
  esp_err_t read_wifi_credentials(wifi_credentials_t *wifi_credentials);
  esp_err_t write_wifi_credentials(const wifi_credentials_t *wifi_credentials);

  void reset_ap_credentials(wifi_credentials_t *wifi_credentials);
  esp_err_t read_ap_credentials(wifi_credentials_t *wifi_credentials);
  esp_err_t write_ap_credentials(const wifi_credentials_t *wifi_credentials);

  esp_err_t read_credentials(app_config_t *app_config);

#ifdef __cplusplus
}
#endif

#endif // __APP_UTILS_H__
