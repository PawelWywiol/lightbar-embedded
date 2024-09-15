#include "app_network.h"

static const char *TAG = "APP_NETWORK";

static void wifi_on_sta_got_ip_handler(void)
{
  ESP_LOGW(TAG, "STA got IP");
}

static void wifi_on_sta_lost_ip_handler(void)
{
  ESP_LOGW(TAG, "STA lost IP");
}

static void wifi_on_sta_start_handler(void)
{
  reconnect_sta(NULL);
}

static wifi_event_handlers_t wifi_handlers = {.on_sta_start = wifi_on_sta_start_handler,
                                              .on_sta_got_ip = wifi_on_sta_got_ip_handler,
                                              .on_sta_lost_ip = wifi_on_sta_lost_ip_handler};

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    if (wifi_handlers.on_sta_start)
      wifi_handlers.on_sta_start();
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    if (wifi_handlers.on_sta_got_ip)
      wifi_handlers.on_sta_got_ip();
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP)
  {
    if (wifi_handlers.on_sta_lost_ip)
      wifi_handlers.on_sta_lost_ip();
  }
}

static esp_err_t init_netif(void)
{
  ESP_LOGI(TAG, "Initializing netif");

  GOTO_CHECK(esp_netif_init(), TAG, "Failed to initialize netif", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_dhcps(void)
{
  ESP_LOGI(TAG, "Initializing DHCP");

  esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

  esp_netif_ip_info_t ip_info = {};

  ip_info.ip.addr = inet_addr(CONFIG_APP_DHCP_IP_START);
  ip_info.netmask.addr = 0x00FFFFFF;
  ip_info.gw.addr = inet_addr(CONFIG_APP_DHCP_IP_START);

  GOTO_CHECK(esp_netif_dhcps_stop(ap_netif), TAG, "Failed to stop DHCP server", error);
  GOTO_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info), TAG, "Failed to set IP info", error);
  GOTO_CHECK(esp_netif_dhcps_start(ap_netif), TAG, "Failed to start DHCP server", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_wifi(void)
{
  ESP_LOGI(TAG, "Initializing WiFi");

  GOTO_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL), TAG,
             "Failed to register WiFi event handler", error);

  GOTO_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL), TAG,
             "Failed to register IP event handler", error);

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  GOTO_CHECK(esp_wifi_init(&config), TAG, "Failed to initialize WiFi", error);
  GOTO_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA), TAG, "Failed to set WiFi mode", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_ap(const wifi_credentials_t *ap_credentials)
{
  ESP_LOGI(TAG, "Initializing AP");

  wifi_config_t wifi_config = {
    .ap =
      {
        .channel = CONFIG_APP_AP_CHANNEL,
        .max_connection = CONFIG_APP_AP_MAX_CONNECTIONS,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
        .authmode = WIFI_AUTH_WPA3_PSK,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
        .authmode = WIFI_AUTH_WPA2_PSK,
#endif
        .pmf_cfg =
          {
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

  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "Failed to set AP config", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_sta(const wifi_credentials_t *ap_credentials, const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Initializing STA");

  esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  esp_netif_set_hostname(netif, ap_credentials->ssid);

  wifi_config_t wifi_config = {
    .sta =
      {
        .scan_method = WIFI_ALL_CHANNEL_SCAN,
        .failure_retry_cnt = CONFIG_APP_STA_MAXIMUM_RETRY,
      },
  };

  strncpy((char *)wifi_config.sta.ssid, wifi_credentials->ssid, sizeof(wifi_config.sta.ssid));
  strncpy((char *)wifi_config.sta.password, wifi_credentials->password, sizeof(wifi_config.sta.password));

  wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
  wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set STA config", error);

  esp_netif_set_default_netif(esp_netif_sta);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_network(const wifi_credentials_t *ap_credentials, const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Initializing network");

  GOTO_CHECK(init_netif(), TAG, "Failed to initialize netif", error);
  GOTO_CHECK(init_dhcps(), TAG, "Failed to initialize DHCP", error);
  GOTO_CHECK(init_wifi(), TAG, "Failed to initialize WiFi", error);
  GOTO_CHECK(init_ap(ap_credentials), TAG, "Failed to initialize AP", error);
  GOTO_CHECK(init_sta(ap_credentials, wifi_credentials), TAG, "Failed to initialize STA", error);

  GOTO_CHECK(esp_wifi_start(), TAG, "Failed to start WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t reconnect_sta(const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Reconnecting STA");

  GOTO_CHECK(esp_wifi_disconnect(), TAG, "Failed to disconnect WiFi", error);

  if (wifi_credentials != NULL)
  {
    wifi_config_t wifi_config = {
      .sta =
        {
          .scan_method = WIFI_ALL_CHANNEL_SCAN,
          .failure_retry_cnt = CONFIG_APP_STA_MAXIMUM_RETRY,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, wifi_credentials->ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, wifi_credentials->password, sizeof(wifi_config.sta.password));

    wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

    GOTO_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set STA config", error);
  }

  GOTO_CHECK(esp_wifi_connect(), TAG, "Failed to connect WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}
