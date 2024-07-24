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

  return readFileData(WIFI_CREDENTIALS_FILE, (char *)wifi_credentials, sizeof(WiFiCredentials));
}

esp_err_t write_wifi_credentials(const WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Writing WiFi credentials");

  return writeFileData(WIFI_CREDENTIALS_FILE, (const char *)wifi_credentials, sizeof(WiFiCredentials));
}
