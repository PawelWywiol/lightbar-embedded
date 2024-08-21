#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

#include "esp_log.h"
#include "esp_http_server.h"

#ifndef FILE_SYSTEM_PATH_MAX_LENGTH
#define FILE_SYSTEM_PATH_MAX_LENGTH 256
#endif

#ifndef FILE_SYSTEM_BASE_PATH
#define FILE_SYSTEM_BASE_PATH "/storage"
#endif

#ifndef FILE_SYSTEM_PUBLIC_BASE_PATH
#define FILE_SYSTEM_PUBLIC_BASE_PATH "/storage/public_html"
#endif

#ifndef SERVER_CONTEXT_BUFFER_MAX_LENGTH
#define SERVER_CONTEXT_BUFFER_MAX_LENGTH (1024 * 10)
#endif

#ifndef SSID_MAX_LENGTH
#define SSID_MAX_LENGTH 32
#endif

#ifndef AP_SSID_MAX_LENGTH
#define AP_SSID_MAX_LENGTH 16
#endif

#ifndef PASSWORD_MAX_LENGTH
#define PASSWORD_MAX_LENGTH 64
#endif

#ifndef UID_MAX_LENGTH
#define UID_MAX_LENGTH 32
#endif

#define TRAILING_ZERO_LENGTH 1

#define SIZE_WITH_TRAILING_ZERO(size) (size + TRAILING_ZERO_LENGTH)

#define GOTO_CHECK(a, tag, str, goto_tag, ...)                                                         \
  do                                                                                                   \
  {                                                                                                    \
    if ((long)(a) != 0)                                                                                \
    {                                                                                                  \
      ESP_LOGE(tag, str " [%s : %d / %s]", __FUNCTION__, __LINE__, esp_err_to_name(a), ##__VA_ARGS__); \
      goto goto_tag;                                                                                   \
    }                                                                                                  \
  } while (0)

typedef struct wifi_credentials
{
  char ssid[SIZE_WITH_TRAILING_ZERO(SSID_MAX_LENGTH)];
  char password[SIZE_WITH_TRAILING_ZERO(PASSWORD_MAX_LENGTH)];
} wifi_credentials_t;

typedef struct app_config
{
  wifi_credentials_t wifi_credentials;
  wifi_credentials_t ap_credentials;
  esp_err_t (*app_api_post_handler)(httpd_req_t *r);
} app_config_t;

#endif // __APP_DEFINES_H__
