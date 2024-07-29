#include "app_nvs.h"

static const char *TAG = "APP_NVS";

esp_err_t nvs_init(void)
{
  ESP_LOGI(TAG, "Initializing NVS");

  esp_err_t err = nvs_flash_init();

  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGE(TAG, "Failed to initialize NVS, erasing and retrying");
    if (nvs_flash_erase() != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to erase NVS");
      return ESP_FAIL;
    }

    if (nvs_flash_init() != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to initialize NVS");
      return ESP_FAIL;
    }
  }

  return err;
}

esp_err_t nvs_read_data(const char *key, void *data, size_t length)
{
  ESP_LOGI(TAG, "Reading data from NVS");

  nvs_handle_t handle;
  esp_err_t err = nvs_open_from_partition(CONFIG_APP_NVS_PARTITION_LABEL, CONFIG_APP_NVS_NAMESPACE, NVS_READONLY, &handle);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace");
    return err;
  }

  err = nvs_get_blob(handle, key, data, &length);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read data from NVS");
  }

  nvs_close(handle);

  return err;
}

esp_err_t nvs_write_data(const char *key, void *data, size_t length)
{
  ESP_LOGI(TAG, "Writing data to NVS");

  nvs_handle_t handle;
  esp_err_t err = nvs_open_from_partition(CONFIG_APP_NVS_PARTITION_LABEL, CONFIG_APP_NVS_NAMESPACE, NVS_READWRITE, &handle);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to open NVS namespace");
    return err;
  }

  err = nvs_set_blob(handle, key, data, length);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to write data to NVS");
  }

  nvs_close(handle);

  return err;
}
