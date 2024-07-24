#ifndef __APP_FILE_SYSTEM_H__
#define __APP_FILE_SYSTEM_H__

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_littlefs.h"

#include "app_defines.h"

bool register_file_system(void);
void unregister_file_system(void);

esp_err_t readFileData(const char *filePath, char *data, size_t size);
esp_err_t writeFileData(const char *filePath, const char *data, size_t size);

esp_err_t createDirectory(const char *directoryPath);

#endif // __APP_FILE_SYSTEM_H__
