#include "app_vfs.h"

static const char *TAG = "APP_FILE_SYSTEM";

esp_vfs_littlefs_conf_t conf = {
    .base_path = FILE_SYSTEM_BASE_PATH,
    .partition_label = CONFIG_APP_FILE_SYSTEM_PARTITION_LABEL,
    .format_if_mount_failed = true,
    .dont_mount = false,
};

esp_err_t init_vfs(void)
{
  ESP_LOGI(TAG, "Initializing file system");

  esp_err_t ret = esp_vfs_littlefs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find LittleFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
    }
    return false;
  }

  return ret;
}

char *clean_vfs_path(char *path)
{
  if (path == NULL)
  {
    return NULL;
  }

  size_t path_length = strlen(path);
  char *new_path = path;

  for (int i = 0; i < path_length; i++)
  {
    if (strchr(ALLOWED_PATH_CHARS, path[i]) == NULL)
    {
      new_path[i] = FORBIDDEN_CHARACTERS_PLACEHOLDER;
    }
  }

  return new_path;
}
