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

esp_err_t readFileData(const char *fileName, char *data, size_t size)
{
  ESP_LOGI(TAG, "Reading file %s", fileName);

  FILE *file = fopen(fileName, "r");

  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file %s", fileName);
    return ESP_FAIL;
  }

  size_t bytesRead = fread(data, 1, size, file);

  if (bytesRead == 0)
  {
    ESP_LOGE(TAG, "Failed to read file %s", fileName);
    fclose(file);
    return ESP_FAIL;
  }

  fclose(file);

  return ESP_OK;
}

esp_err_t writeFileData(const char *fileName, const char *data, size_t size)
{
  ESP_LOGI(TAG, "Writing file %s", fileName);

  FILE *file = fopen(fileName, "w");

  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file %s", fileName);
    return ESP_FAIL;
  }

  size_t bytesWritten = fwrite(data, 1, size, file);

  if (bytesWritten == 0)
  {
    ESP_LOGE(TAG, "Failed to write file %s", fileName);
    fclose(file);
    return ESP_FAIL;
  }

  fclose(file);

  return ESP_OK;
}

esp_err_t createDirectory(const char *directoryPath)
{
  struct stat st;
  if (stat(directoryPath, &st) == -1)
  {
    ESP_LOGI(TAG, "Creating directory %s", directoryPath);
    if (mkdir(directoryPath, 0775) == -1)
    {
      ESP_LOGE(TAG, "Failed to create directory %s", directoryPath);
      return ESP_FAIL;
    }
  }

  return ESP_OK;
}
