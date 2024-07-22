#include "app_network.h"

static const char *TAG = "APP_NETWORK";

void reset_wifi_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Resetting WiFi credentials");

  memset(wifi_credentials, 0, sizeof(WiFiCredentials));
}

esp_err_t read_wifi_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Reading WiFi credentials");

  FILE *file = fopen(WIFI_CREDENTIALS_FILE, "r");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return ESP_FAIL;
  }

  size_t bytes_read = fread(wifi_credentials, sizeof(WiFiCredentials), 1, file);
  fclose(file);

  if (bytes_read != 1)
  {
    ESP_LOGE(TAG, "Failed to read WiFi credentials");
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t write_wifi_credentials(const WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Writing WiFi credentials");

  FILE *file = fopen(WIFI_CREDENTIALS_FILE, "w");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return ESP_FAIL;
  }

  size_t bytes_written = fwrite(wifi_credentials, sizeof(WiFiCredentials), 1, file);
  fclose(file);

  if (bytes_written != 1)
  {
    ESP_LOGE(TAG, "Failed to write WiFi credentials");
    return ESP_FAIL;
  }

  return ESP_OK;
}
