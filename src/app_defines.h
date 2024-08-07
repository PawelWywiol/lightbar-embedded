#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

// Common defines

#ifndef FILE_SYSTEM_BASE_PATH_MAX_LENGTH
#define FILE_SYSTEM_BASE_PATH_MAX_LENGTH 32
#endif

#ifndef FILE_SYSTEM_PATH_MAX_LENGTH
#define FILE_SYSTEM_PATH_MAX_LENGTH 256
#endif

#ifndef FILE_SYSTEM_BASE_PATH
#define FILE_SYSTEM_BASE_PATH "/storage"
#endif

#ifndef FILE_SYSTEM_CONFIG_BASE_PATH
#define FILE_SYSTEM_CONFIG_BASE_PATH "/config"
#endif

#ifndef FILE_SYSTEM_PUBLIC_BASE_PATH
#define FILE_SYSTEM_PUBLIC_BASE_PATH "/public_html"
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

#ifndef UUID_MAX_LENGTH
#define UUID_MAX_LENGTH 36
#endif

#define TRAILING_ZERO_LENGTH 1

#define SIZE_WITH_TRAILING_ZERO(size) (size + TRAILING_ZERO_LENGTH)

#define GOTO_CHECK(a, tag, str, goto_tag, ...)                              \
  do                                                                        \
  {                                                                         \
    if (!(a))                                                               \
    {                                                                       \
      ESP_LOGE(tag, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      goto goto_tag;                                                        \
    }                                                                       \
  } while (0)

#endif // __APP_DEFINES_H__
