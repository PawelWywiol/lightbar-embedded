#ifndef __APP_VFS_SYSTEM_H__
#define __APP_VFS_SYSTEM_H__

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_littlefs.h"

#include "app_defines.h"

#define ALLOWED_PATH_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_./"
#define FORBIDDEN_CHARACTERS_PLACEHOLDER '-'
#define TRIM_SLASHES(path) (path[0] == '/' ? path + 1 : path)

typedef struct vfs_size
{
  size_t total;
  size_t used;
  size_t free;
} vfs_size_t;

esp_err_t init_vfs(void);

char *clean_vfs_path(char *path);
vfs_size_t get_vfs_space_info(void);

#endif // __APP_VFS_SYSTEM_H__
