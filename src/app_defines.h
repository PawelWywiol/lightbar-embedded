#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

// Common defines

#ifndef FILE_SYSTEM_BASE_PATH
#define FILE_SYSTEM_BASE_PATH "/storage"
#endif

#ifndef FILE_SYSTEM_MAX_PATH
#define FILE_SYSTEM_MAX_PATH 256
#endif

#ifndef FILE_SYSTEM_CONFIG_DIRECTORY_PATH
#define FILE_SYSTEM_CONFIG_DIRECTORY_PATH "/config"
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

#endif // __APP_DEFINES_H__
