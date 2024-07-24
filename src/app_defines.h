#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

// General application defines

#ifndef APP_DELAY
#define APP_DELAY 500
#endif

// File system defines

#ifndef APP_FILE_SYSTEM_BASE_PATH
#define APP_FILE_SYSTEM_BASE_PATH "/storage"
#endif

#ifndef APP_FILE_SYSTEM_PARTITION_LABEL
#define APP_FILE_SYSTEM_PARTITION_LABEL "storage"
#endif

#ifndef APP_FILE_SYSTEM_FORMAT_IF_MOUNT_FAILED
#define APP_FILE_SYSTEM_FORMAT_IF_MOUNT_FAILED true
#endif

#ifndef APP_FILE_SYSTEM_DONT_MOUNT
#define APP_FILE_SYSTEM_DONT_MOUNT false
#endif

#ifndef APP_FILE_SYSTEM_MAX_PATH
#define APP_FILE_SYSTEM_MAX_PATH 256
#endif

#ifndef APP_FILE_SYSTEM_CONFIG_DIRECTORY_PATH
#define APP_FILE_SYSTEM_CONFIG_DIRECTORY_PATH "/config"
#endif

// WiFi credentials defines

#ifndef WIFI_CREDENTIALS_FILE
#define WIFI_CREDENTIALS_FILE "/config/wifi_credentials"
#endif

// Common defines

#ifndef SSID_MAX_LENGTH
#define SSID_MAX_LENGTH 32
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
