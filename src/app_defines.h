#ifndef __APP_DEFINES_H__
#define __APP_DEFINES_H__

// General application defines

#ifndef APP_DELAY
#define APP_DELAY 500
#endif

// File system defines

#ifndef APP_FILE_SYSTEM_BASE_PATH
#define APP_FILE_SYSTEM_BASE_PATH "/"
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

#endif // __APP_DEFINES_H__
