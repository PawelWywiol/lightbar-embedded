#include "app_network.h"

static const char *TAG = "APP_NETWORK";
static const char *TAG_AP = "APP_NETWORK_AP";
static const char *TAG_STA = "APP_NETWORK_STA";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG_AP, "Station " MACSTR " left, AID=%d, reason:%d",
             MAC2STR(event->mac), event->aid, event->reason);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    ESP_LOGI(TAG_STA, "Station started");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
  }
}

static esp_err_t init_netif() {
  ESP_LOGI(TAG, "Initializing netif");

  GOTO_CHECK(esp_netif_init(), TAG, "Failed to initialize netif", error);

  ESP_LOGI(TAG, "Netif initialized");

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_dhcps() {
  ESP_LOGI(TAG, "Initializing DHCP");

  esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

  esp_netif_ip_info_t ip_info = {};

  ip_info.ip.addr = inet_addr(CONFIG_APP_DHCP_IP_START);
  ip_info.netmask.addr = 0x00FFFFFF;
  ip_info.gw.addr = inet_addr(CONFIG_APP_DHCP_IP_START);

  GOTO_CHECK(esp_netif_dhcps_stop(ap_netif), TAG, "Failed to stop DHCP server",
             error);
  GOTO_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info), TAG,
             "Failed to set IP info", error);
  GOTO_CHECK(esp_netif_dhcps_start(ap_netif), TAG,
             "Failed to start DHCP server", error);

  ESP_LOGI(TAG, "DHCP initialized");

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_mdns() {
  ESP_LOGI(TAG, "Initializing MDNS");

  GOTO_CHECK(mdns_init(), TAG, "Failed to initialize MDNS", error);
  GOTO_CHECK(mdns_hostname_set(CONFIG_APP_MDNS_HOST_NAME), TAG,
             "Failed to set MDNS hostname", error);
  GOTO_CHECK(mdns_instance_name_set(CONFIG_APP_MDNS_INSTANCE_NAME), TAG,
             "Failed to set MDNS instance name", error);

  mdns_txt_item_t serviceData[] = {{"board", "esp32"}, {"path", "/"}};
  size_t serviceDataCount = sizeof(serviceData) / sizeof(serviceData[0]);

  GOTO_CHECK(mdns_service_add(CONFIG_APP_MDNS_INSTANCE_NAME, "_http", "_tcp",
                              80, serviceData, serviceDataCount),
             TAG, "Failed to add MDNS service", error);

  ESP_LOGI(TAG, "MDNS service started");

  return ESP_OK;
error:
  return ESP_FAIL;
}

static esp_err_t init_netbios() {
  ESP_LOGI(TAG, "Initializing NetBIOS");

  netbiosns_init();
  netbiosns_set_name(CONFIG_APP_MDNS_HOST_NAME);

  ESP_LOGI(TAG, "NetBIOS service started");

  return ESP_OK;
}

static esp_err_t init_wifi() {
  ESP_LOGI(TAG, "Initializing WiFi");

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  GOTO_CHECK(esp_wifi_init(&config), TAG, "Failed to initialize WiFi", error);

  GOTO_CHECK(esp_event_handler_instance_register(
                 WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL),
             TAG, "Failed to register WiFi event handler", error);

  GOTO_CHECK(esp_event_handler_instance_register(
                 IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL),
             TAG, "Failed to register IP event handler", error);

  ESP_LOGI(TAG, "WiFi initialized");

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_network() {
  ESP_LOGI(TAG, "Initializing network");

  GOTO_CHECK(init_netif(), TAG, "Failed to initialize netif", error);
  GOTO_CHECK(init_mdns(), TAG, "Failed to initialize MDNS", error);
  GOTO_CHECK(init_netbios(), TAG, "Failed to initialize NetBIOS", error);
  GOTO_CHECK(init_dhcps(), TAG, "Failed to initialize DHCP", error);
  GOTO_CHECK(init_wifi(), TAG, "Failed to initialize WiFi", error);

  ESP_LOGI(TAG, "Network initialized");

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_ap(const wifi_credentials_t *ap_credentials) {
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

  strncpy((char *)wifi_config.ap.ssid, ap_credentials->ssid,
          sizeof(wifi_config.ap.ssid));
  strncpy((char *)wifi_config.ap.password, ap_credentials->password,
          sizeof(wifi_config.ap.password));

  wifi_config.ap.password[sizeof(wifi_config.ap.password) - 1] = '\0';
  wifi_config.ap.ssid[sizeof(wifi_config.ap.ssid) - 1] = '\0';
  wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);

  if (strlen(ap_credentials->password) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  GOTO_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG_AP,
             "Failed to set WiFi mode", error);

  ESP_LOGI(TAG, "AP initialized");

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t start_wifi() {
  ESP_LOGI(TAG, "Starting WiFi");

  GOTO_CHECK(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed to set WiFi mode",
             error);
  GOTO_CHECK(esp_wifi_start(), TAG, "Failed to start WiFi", error);

  ESP_LOGI(TAG, "WiFi started");

  return ESP_OK;
error:
  return ESP_FAIL;
}
