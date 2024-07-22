#include "app_file_system.h"

static const char *TAG = "APP_FILE_SYSTEM";

esp_vfs_littlefs_conf_t conf = {
    .base_path = APP_FILE_SYSTEM_BASE_PATH,
    .partition_label = APP_FILE_SYSTEM_PARTITION_LABEL,
    .format_if_mount_failed = APP_FILE_SYSTEM_FORMAT_IF_MOUNT_FAILED,
    .dont_mount = APP_FILE_SYSTEM_DONT_MOUNT,
};

bool register_file_system(void)
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

  return true;
}

void unregister_file_system(void)
{
  ESP_LOGI(TAG, "Deinitializing file system");

  esp_err_t ret = esp_vfs_littlefs_unregister(conf.base_path);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to deinitialize LittleFS (%s)", esp_err_to_name(ret));
  }
}
