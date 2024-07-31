#include "app_file_system.h"

static const char *TAG = "APP_FILE_SYSTEM";

esp_vfs_littlefs_conf_t conf = {
    .base_path = FILE_SYSTEM_BASE_PATH,
    .partition_label = CONFIG_APP_FILE_SYSTEM_PARTITION_LABEL,
    .format_if_mount_failed = true,
    .dont_mount = false,
};

esp_err_t register_file_system(void)
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

void unregister_file_system(void)
{
  ESP_LOGI(TAG, "Deinitializing file system");

  esp_err_t ret = esp_vfs_littlefs_unregister(conf.base_path);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to deinitialize LittleFS (%s)", esp_err_to_name(ret));
  }
}

esp_err_t read_file_data(const char *filePath, char *data, size_t size)
{
  char path[FILE_SYSTEM_MAX_PATH] = {0};
  snprintf(path, FILE_SYSTEM_MAX_PATH, "%s/%s", FILE_SYSTEM_BASE_PATH, TRIM_SLASHES(filePath));

  ESP_LOGI(TAG, "Reading file %s", path);

  FILE *file = fopen(path, "r");

  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file %s", path);
    return ESP_FAIL;
  }

  size_t bytesRead = fread(data, 1, size, file);

  if (bytesRead == 0)
  {
    ESP_LOGE(TAG, "Failed to read file %s", path);
    fclose(file);
    return ESP_FAIL;
  }

  fclose(file);

  return ESP_OK;
}

esp_err_t write_file_data(const char *filePath, const char *data, size_t size)
{
  char path[FILE_SYSTEM_MAX_PATH] = {0};
  snprintf(path, FILE_SYSTEM_MAX_PATH, "%s/%s", FILE_SYSTEM_BASE_PATH, TRIM_SLASHES(filePath));

  ESP_LOGI(TAG, "Writing file %s", path);

  FILE *file = fopen(path, "w");

  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file %s", path);
    return ESP_FAIL;
  }

  size_t bytesWritten = fwrite(data, 1, size, file);

  if (bytesWritten == 0)
  {
    ESP_LOGE(TAG, "Failed to write file %s", path);
    fclose(file);
    return ESP_FAIL;
  }

  fclose(file);

  return ESP_OK;
}

esp_err_t create_directory(const char *directoryPath)
{
  char path[FILE_SYSTEM_MAX_PATH] = {0};
  snprintf(path, FILE_SYSTEM_MAX_PATH, "%s/%s", FILE_SYSTEM_BASE_PATH, TRIM_SLASHES(directoryPath));

  struct stat st;
  if (stat(path, &st) == -1)
  {
    ESP_LOGI(TAG, "Creating directory %s", path);
    if (mkdir(path, 0775) == -1)
    {
      ESP_LOGE(TAG, "Failed to create directory %s", path);
      return ESP_FAIL;
    }
  }

  return ESP_OK;
}

esp_err_t create_config_directory()
{
  return create_directory(FILE_SYSTEM_CONFIG_DIRECTORY_PATH);
}
