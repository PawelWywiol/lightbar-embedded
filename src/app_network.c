#include "app_network.h"

static const char *TAG = "APP_NETWORK";
static const char *TAG_AP = "APP_NETWORK_AP";
static const char *TAG_STA = "APP_NETWORK_STA";

static char public_ip[IP4_ADDR_STRLEN_MAX] = {0};

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    switch (event_id)
    {
    case WIFI_EVENT_STA_START: {
      esp_wifi_connect();
      ESP_LOGI(TAG_STA, "Station started");
    }
    break;
    case WIFI_EVENT_STA_STOP: {
      ESP_LOGI(TAG_STA, "Station stopped");
    }
    break;
    case WIFI_EVENT_STA_CONNECTED: {
      wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
      ESP_LOGI(TAG_STA, "Station connected to %s, AID=%d", (char *)event->ssid, event->aid);
    }
    break;
    case WIFI_EVENT_STA_DISCONNECTED: {
      wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
      ESP_LOGI(TAG_STA, "Station disconnected from %s, reason:%d", (char *)event->ssid, event->reason);
    }
    break;
    case WIFI_EVENT_AP_START: {
      ESP_LOGI(TAG_AP, "Access point started");
    }
    break;
    case WIFI_EVENT_AP_STOP: {
      ESP_LOGI(TAG_AP, "Access point stopped");
    }
    break;
    case WIFI_EVENT_AP_STACONNECTED: {
      wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
      ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
    }
    break;
    case WIFI_EVENT_AP_STADISCONNECTED: {
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
      ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
    }
    break;
    case WIFI_EVENT_STA_BEACON_TIMEOUT: {
      ESP_LOGI(TAG_STA, "Beacon timeout");
    }
    break;
    case WIFI_EVENT_HOME_CHANNEL_CHANGE: {
      wifi_event_home_channel_change_t *event = (wifi_event_home_channel_change_t *)event_data;
      ESP_LOGI(TAG, "Home channel changed from %d to %d", event->old_chan, event->new_chan);
    }
    break;
    default:
      ESP_LOGI(TAG, "Unknown event [%s][%ld]", event_base, event_id);
      break;
    }
  }
  else if (event_base == IP_EVENT)
  {
    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP: {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));

      memset((char *)public_ip, 0, IP4_ADDR_STRLEN_MAX);
      snprintf((char *)public_ip, IP4_ADDR_STRLEN_MAX, IPSTR, IP2STR(&event->ip_info.ip));
    }
    break;
    case IP_EVENT_STA_LOST_IP: {
      ESP_LOGI(TAG_STA, "Lost IP");

      memset((char *)public_ip, 0, IP4_ADDR_STRLEN_MAX);
    }
    break;
    case IP_EVENT_AP_STAIPASSIGNED: {
      ESP_LOGI(TAG_AP, "Station assigned IP");
    }
    break;
    default:
      ESP_LOGI(TAG, "Unknown event [%s][%ld]", event_base, event_id);
      break;
    }
  }
  else
  {
    ESP_LOGI(TAG, "Unknown event [%s][%ld]", event_base, event_id);
  }
}

static esp_err_t init_netif()
{
  ESP_LOGI(TAG, "Initializing netif");

  GOTO_CHECK(esp_netif_init(), TAG, "Failed to initialize netif", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_dhcps()
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

static esp_err_t init_mdns()
{
  ESP_LOGI(TAG, "Initializing MDNS");

  GOTO_CHECK(mdns_init(), TAG, "Failed to initialize MDNS", error);
  GOTO_CHECK(mdns_hostname_set(CONFIG_APP_MDNS_HOST_NAME), TAG, "Failed to set MDNS hostname", error);
  GOTO_CHECK(mdns_instance_name_set(CONFIG_APP_MDNS_INSTANCE_NAME), TAG, "Failed to set MDNS instance name", error);

  mdns_txt_item_t serviceData[] = {{"board", "esp32"}, {"path", "/"}};
  size_t serviceDataCount = sizeof(serviceData) / sizeof(serviceData[0]);

  GOTO_CHECK(mdns_service_add(CONFIG_APP_MDNS_INSTANCE_NAME, "_http", "_tcp", 80, serviceData, serviceDataCount), TAG,
             "Failed to add MDNS service", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_netbios()
{
  ESP_LOGI(TAG, "Initializing NetBIOS");

  netbiosns_init();
  netbiosns_set_name(CONFIG_APP_MDNS_HOST_NAME);

  return ESP_OK;
}

static esp_err_t init_wifi()
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

esp_err_t init_network()
{
  ESP_LOGI(TAG, "Initializing network");

  GOTO_CHECK(init_netif(), TAG, "Failed to initialize netif", error);
  GOTO_CHECK(init_mdns(), TAG, "Failed to initialize MDNS", error);
  GOTO_CHECK(init_netbios(), TAG, "Failed to initialize NetBIOS", error);
  GOTO_CHECK(init_dhcps(), TAG, "Failed to initialize DHCP", error);
  GOTO_CHECK(init_wifi(), TAG, "Failed to initialize WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_ap(const wifi_credentials_t *ap_credentials)
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

  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG_AP, "Failed to set WiFi mode", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_sta(const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Initializing STA");

  esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

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

  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG_STA, "Failed to set WiFi mode", error);

  esp_netif_set_default_netif(esp_netif_sta);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t start_wifi()
{
  ESP_LOGI(TAG, "Starting WiFi");

  GOTO_CHECK(esp_wifi_start(), TAG, "Failed to start WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t reconnect_sta(const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Reconnecting STA");

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

  GOTO_CHECK(esp_wifi_disconnect(), TAG_STA, "Failed to disconnect WiFi", error);
  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG_STA, "Failed to set WiFi mode", error);
  GOTO_CHECK(esp_wifi_connect(), TAG_STA, "Failed to connect WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}
