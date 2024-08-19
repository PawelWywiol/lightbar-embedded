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

  GOTO_CHECK(esp_vfs_littlefs_register(&conf) != ESP_OK, TAG, "Failed to register LittleFS", error);

  return ESP_OK;
error:
  return ESP_FAIL;
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
