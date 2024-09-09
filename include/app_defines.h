#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

#include "esp_log.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum connection_request_type_info
  {
    CONNECTION_REQUEST_TYPE_NONE = 0,
    CONNECTION_REQUEST_WIFI_INFO = 0x77696669,
    CONNECTION_REQUEST_COLORS_INFO = 0x636f6c6f,
    CONNECTION_REQUEST_FRAME_INFO = 0x6672616d,
    CONNECTION_REQUEST_EOL_INFO = 0x454f4c00
  } connection_request_type_info_t;

#define CONNECTION_REQUEST_TYPE_INFO_LENGTH 4
#define CONNECTION_REQUEST_SIZE_INFO_LENGTH 4
#define CONNECTION_REQUEST_EOL_INFO_LENGTH 4
#define CONNECTION_REQUEST_INFO_LENGTH 12

#ifndef FILE_SYSTEM_PATH_MAX_LENGTH
#define FILE_SYSTEM_PATH_MAX_LENGTH 256
#endif

#ifndef FILE_SYSTEM_BASE_PATH
#define FILE_SYSTEM_BASE_PATH "/storage"
#endif

#ifndef FILE_SYSTEM_PUBLIC_BASE_PATH
#define FILE_SYSTEM_PUBLIC_BASE_PATH "/storage/public_html"
#endif

#ifndef FILE_SYSTEM_TEMP_BASE_PATH
#define FILE_SYSTEM_TEMP_BASE_PATH "/storage/temp"
#endif

#ifndef CONTEXT_BUFFER_MAX_LENGTH
#define CONTEXT_BUFFER_MAX_LENGTH (1024 * 10)
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

#define IP4_ADDR_STRLEN_MAX 16

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
  } app_config_t;

#ifdef __cplusplus
}
#endif

#endif // __APP_DEFINES_H__
