#include "app_utils.h"
#include "esp_mac.h"

static const char uid_chars[] = "0123456789abcdefghhilmnopqrstuvwxyz";

static const char *TAG = "APP_UTILS";

void uid(char *uid, size_t length)
{
  ESP_LOGI(TAG, "Generating UID");

  for (size_t i = 0; i < length; i++)
  {
    uid[i] = uid_chars[rand() % (sizeof(uid_chars) - 1)];
  }

  uid[length - 1] = '\0';
}

void reset_wifi_credentials(wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Resetting WiFi credentials");

  memset(wifi_credentials, 0, sizeof(wifi_credentials_t));
}

esp_err_t write_wifi_credentials(const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Writing WiFi credentials");

  return nvs_write_data(CONFIG_APP_NVS_WIFI_CREDENTIALS_KEY, (void *)wifi_credentials, sizeof(wifi_credentials_t));
}

esp_err_t read_wifi_credentials(wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Reading WiFi credentials");

  if (nvs_read_data(CONFIG_APP_NVS_WIFI_CREDENTIALS_KEY, (void *)wifi_credentials, sizeof(wifi_credentials_t)) !=
      ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read WiFi credentials");

    reset_wifi_credentials(wifi_credentials);

    if (write_wifi_credentials(wifi_credentials) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to write WiFi credentials");

      return ESP_FAIL;
    }
  }

  ESP_LOGI(TAG, "WiFi ssid : %s", wifi_credentials->ssid);

  return ESP_OK;
}

void reset_ap_credentials(wifi_credentials_t *ap_credentials)
{
  ESP_LOGI(TAG, "Resetting AP credentials");

  memset(ap_credentials, 0, sizeof(wifi_credentials_t));

  strncpy(ap_credentials->password, CONFIG_APP_AP_PASSWORD, strlen(CONFIG_APP_AP_PASSWORD));
  ap_credentials->password[strlen(CONFIG_APP_AP_PASSWORD)] = '\0';
}

// derived on every boot, not stored: older firmware saved the same name on every board
static void set_ap_ssid_from_mac(wifi_credentials_t *ap_credentials)
{
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

  snprintf(ap_credentials->ssid, sizeof(ap_credentials->ssid), "%s-%02x%02x%02x", CONFIG_APP_AP_SSID_PREFIX, mac[3],
           mac[4], mac[5]);
}

esp_err_t write_ap_credentials(const wifi_credentials_t *ap_credentials)
{
  ESP_LOGI(TAG, "Writing AP credentials");

  return nvs_write_data(CONFIG_APP_NVS_AP_CREDENTIALS_KEY, (void *)ap_credentials, sizeof(wifi_credentials_t));
}

esp_err_t read_ap_credentials(wifi_credentials_t *ap_credentials)
{
  ESP_LOGI(TAG, "Reading AP credentials");

  if (nvs_read_data(CONFIG_APP_NVS_AP_CREDENTIALS_KEY, (void *)ap_credentials, sizeof(wifi_credentials_t)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read AP credentials");

    reset_ap_credentials(ap_credentials);

    if (write_ap_credentials(ap_credentials) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to write AP credentials");

      return ESP_FAIL;
    }
  }

  set_ap_ssid_from_mac(ap_credentials);

  ESP_LOGI(TAG, "AP ssid : %s", ap_credentials->ssid);

  return ESP_OK;
}
