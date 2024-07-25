#include "app_network.h"

static const char uid_chars[] = "0123456789abcdefghhilmnopqrstuvwxyz";
static const char *TAG = "APP_NETWORK";

void uid(char *uid, size_t length)
{
  ESP_LOGI(TAG, "Generating UID");

  for (size_t i = 0; i < length; i++)
  {
    uid[i] = uid_chars[rand() % (sizeof(uid_chars) - 1)];
  }

  uid[length] = '\0';
}

void reset_wifi_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Resetting WiFi credentials");

  memset(wifi_credentials, 0, sizeof(WiFiCredentials));
}

esp_err_t write_wifi_credentials(const WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Writing WiFi credentials");

  return writeFileData(APP_WIFI_CREDENTIALS_FILE, (const char *)wifi_credentials, sizeof(WiFiCredentials));
}

esp_err_t read_wifi_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Reading WiFi credentials");

  if (readFileData(APP_WIFI_CREDENTIALS_FILE, (char *)wifi_credentials, sizeof(WiFiCredentials)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read WiFi credentials");

    reset_wifi_credentials(wifi_credentials);

    if (write_wifi_credentials(wifi_credentials) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to write WiFi credentials");

      return ESP_FAIL;
    }
  }

  return ESP_OK;
}

void reset_ap_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Resetting AP credentials");

  memset(wifi_credentials, 0, sizeof(WiFiCredentials));

  uid(wifi_credentials->ssid, APP_AP_SSID_LENGTH);

  strncpy(wifi_credentials->ssid, APP_AP_SSID_PREFIX, strlen(APP_AP_SSID_PREFIX));
  wifi_credentials->ssid[strlen(APP_AP_SSID_PREFIX)] = '-';
  wifi_credentials->ssid[APP_AP_SSID_LENGTH] = '\0';

  strncpy(wifi_credentials->password, APP_AP_PASSWORD, strlen(APP_AP_PASSWORD));
  wifi_credentials->password[strlen(APP_AP_PASSWORD)] = '\0';
}

esp_err_t write_ap_credentials(const WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Writing AP credentials");

  return writeFileData(APP_AP_CREDENTIALS_FILE, (const char *)wifi_credentials, sizeof(WiFiCredentials));
}

esp_err_t read_ap_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Reading AP credentials");

  if (readFileData(APP_AP_CREDENTIALS_FILE, (char *)wifi_credentials, sizeof(WiFiCredentials)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read AP credentials");

    reset_ap_credentials(wifi_credentials);

    if (write_wifi_credentials(wifi_credentials) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to write AP credentials");

      return ESP_FAIL;
    }
  }

  return ESP_OK;
}
