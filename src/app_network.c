#include "app_network.h"

static const char uid_chars[] = "0123456789abcdefghhilmnopqrstuvwxyz";
static const char *TAG = "APP_NETWORK";
static const char *TAG_AP = "APP_NETWORK_AP";
static const char *TAG_STA = "APP_NETWORK_STA";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d",
             MAC2STR(event->mac), event->aid);
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID=%d, reason:%d",
             MAC2STR(event->mac), event->aid, event->reason);
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
    ESP_LOGI(TAG_STA, "Station started");
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
  }
}

esp_err_t init_wifi()
{
  esp_err_t err = esp_netif_init();

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize netif (%s)", esp_err_to_name(err));
    return err;
  }

  err = esp_event_loop_create_default();

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create event loop (%s)", esp_err_to_name(err));
    return err;
  }

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&config);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize WiFi (%s)", esp_err_to_name(err));
    return err;
  }

  err = esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &wifi_event_handler,
                                            NULL,
                                            NULL);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register WiFi event handler (%s)", esp_err_to_name(err));
    return err;
  }

  err = esp_event_handler_instance_register(IP_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &wifi_event_handler,
                                            NULL,
                                            NULL);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register IP event handler (%s)", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

esp_netif_t *init_ap(const WiFiCredentials *ap_credentials)
{
  esp_netif_t *esp_netif = esp_netif_create_default_wifi_ap();

  wifi_config_t wifi_config = {
      .ap = {
          .channel = CONFIG_APP_AP_CHANNEL,
          .max_connection = CONFIG_APP_AP_MAX_CONNECTIONS,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
          .authmode = WIFI_AUTH_WPA3_PSK,
          .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
          .authmode = WIFI_AUTH_WPA2_PSK,
#endif
          .pmf_cfg = {
              .required = true,
          },
      },
  };

  strncpy((char *)wifi_config.ap.ssid, ap_credentials->ssid, sizeof(wifi_config.ap.ssid));
  strncpy((char *)wifi_config.ap.password, ap_credentials->password, sizeof(wifi_config.ap.password));

  wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
  wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
  wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);

  if (strlen(ap_credentials->password) == 0)
  {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

  return esp_netif;
}

esp_err_t start_wifi()
{
  esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set WiFi mode (%s)", esp_err_to_name(err));
    return err;
  }

  err = esp_wifi_start();

  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to start WiFi (%s)", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

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

  return nvs_write_data(CONFIG_APP_NVS_WIFI_CREDENTIALS_KEY, (void *)wifi_credentials, sizeof(WiFiCredentials));
}

esp_err_t read_wifi_credentials(WiFiCredentials *wifi_credentials)
{
  ESP_LOGI(TAG, "Reading WiFi credentials");

  if (nvs_read_data(CONFIG_APP_NVS_WIFI_CREDENTIALS_KEY, (void *)wifi_credentials, sizeof(WiFiCredentials)) != ESP_OK)
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

void reset_ap_credentials(WiFiCredentials *ap_credentials)
{
  ESP_LOGI(TAG, "Resetting AP credentials");

  memset(ap_credentials, 0, sizeof(WiFiCredentials));

  uid(ap_credentials->ssid, AP_SSID_MAX_LENGTH);

  strncpy(ap_credentials->ssid, CONFIG_APP_AP_SSID_PREFIX, strlen(CONFIG_APP_AP_SSID_PREFIX));
  ap_credentials->ssid[strlen(CONFIG_APP_AP_SSID_PREFIX)] = '-';
  ap_credentials->ssid[AP_SSID_MAX_LENGTH] = '\0';

  strncpy(ap_credentials->password, CONFIG_APP_AP_PASSWORD, strlen(CONFIG_APP_AP_PASSWORD));
  ap_credentials->password[strlen(CONFIG_APP_AP_PASSWORD)] = '\0';
}

esp_err_t write_ap_credentials(const WiFiCredentials *ap_credentials)
{
  ESP_LOGI(TAG, "Writing AP credentials");

  return nvs_write_data(CONFIG_APP_NVS_AP_CREDENTIALS_KEY, (void *)ap_credentials, sizeof(WiFiCredentials));
}

esp_err_t read_ap_credentials(WiFiCredentials *ap_credentials)
{
  ESP_LOGI(TAG, "Reading AP credentials");

  if (nvs_read_data(CONFIG_APP_NVS_AP_CREDENTIALS_KEY, (void *)ap_credentials, sizeof(WiFiCredentials)) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to read AP credentials");

    reset_ap_credentials(ap_credentials);

    if (write_ap_credentials(ap_credentials) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to write AP credentials");

      return ESP_FAIL;
    }
  }

  return ESP_OK;
}
