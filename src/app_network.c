#include "app_network.h"

static const char *TAG = "APP_NETWORK";

static esp_timer_handle_t sta_retry_timer = NULL;
static uint32_t sta_retry_count = 0;
static sta_state_t sta_state = STA_STATE_CONNECTING;
static uint16_t sta_last_reason = 0;
static uint32_t sta_drops = 0;
static uint16_t sta_last_drop_reason = 0;
static bool sta_is_attempting = false;
static bool sta_is_leaving = false;

// esp_wifi reads up to 32 SSID bytes without null terminator (802.11 max length)
static size_t copy_ssid(uint8_t *destination, const char *source)
{
  size_t length = strnlen(source, SSID_MAX_LENGTH);
  memcpy(destination, source, length);
  return length;
}

static bool has_sta_credentials(void)
{
  wifi_config_t wifi_config = {0};
  return esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK && wifi_config.sta.ssid[0] != '\0';
}

static void start_sta_retry_timer(uint32_t seconds)
{
  esp_timer_stop(sta_retry_timer);
  esp_timer_start_once(sta_retry_timer, (uint64_t)seconds * 1000 * 1000);
}

static uint32_t get_sta_retry_delay(void)
{
  uint32_t delay = sta_retry_count < 8 ? 1U << sta_retry_count : CONFIG_APP_STA_RETRY_MAX_DELAY;
  return delay < CONFIG_APP_STA_RETRY_MAX_DELAY ? delay : CONFIG_APP_STA_RETRY_MAX_DELAY;
}

static void retry_sta(void *arg)
{
  sta_retry_count++;
  ESP_LOGI(TAG, "STA reconnect attempt %lu", (unsigned long)sta_retry_count);

  start_sta_retry_timer(CONFIG_APP_STA_ATTEMPT_TIMEOUT);
  reconnect_sta(NULL);
}

static void wifi_on_sta_connected_handler(void)
{
  ESP_LOGI(TAG, "STA connected to AP");

  sta_state = STA_STATE_WAITING_FOR_IP;
  start_sta_retry_timer(CONFIG_APP_STA_ATTEMPT_TIMEOUT);
}

static void wifi_on_sta_got_ip_handler(void)
{
  ESP_LOGI(TAG, "STA got IP");

  sta_state = STA_STATE_CONNECTED;
  sta_last_reason = 0;
  sta_retry_count = 0;
  esp_timer_stop(sta_retry_timer);
}

static void wifi_on_sta_disconnected_handler(const wifi_event_sta_disconnected_t *event)
{
  ESP_LOGI(TAG, "STA disconnected from AP, reason %u", event->reason);

  if (sta_is_leaving)
  {
    sta_is_leaving = false;
    sta_state = STA_STATE_CONNECTING;

    if (!esp_timer_is_active(sta_retry_timer))
    {
      start_sta_retry_timer(CONFIG_APP_STA_ATTEMPT_TIMEOUT);
    }

    return;
  }

  sta_is_attempting = false;
  sta_last_reason = event->reason;

  if (sta_state == STA_STATE_CONNECTED)
  {
    sta_drops++;
    sta_last_drop_reason = event->reason;
  }

  sta_state = STA_STATE_CONNECTING;

  if (has_sta_credentials())
  {
    start_sta_retry_timer(get_sta_retry_delay());
  }
}

static void wifi_on_sta_start_handler(void)
{
  reconnect_sta(NULL);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  ESP_LOGI(TAG, "WiFi event : %s [%lu]", event_base, event_id);

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    wifi_on_sta_start_handler();
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
  {
    wifi_on_sta_connected_handler();
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    wifi_on_sta_disconnected_handler((const wifi_event_sta_disconnected_t *)event_data);
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    wifi_on_sta_got_ip_handler();
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

  const esp_timer_create_args_t timer_args = {.callback = &retry_sta, .name = "sta_retry_timer"};
  GOTO_CHECK(esp_timer_create(&timer_args, &sta_retry_timer), TAG, "Failed to create STA retry timer", error);

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

  wifi_config.ap.ssid_len = copy_ssid(wifi_config.ap.ssid, ap_credentials->ssid);
  strlcpy((char *)wifi_config.ap.password, ap_credentials->password, sizeof(wifi_config.ap.password));

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

  copy_ssid(wifi_config.sta.ssid, wifi_credentials->ssid);
  strlcpy((char *)wifi_config.sta.password, wifi_credentials->password, sizeof(wifi_config.sta.password));

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
  GOTO_CHECK(esp_wifi_set_ps(WIFI_PS_NONE), TAG, "Failed to disable WiFi power save", error);
#if CONFIG_APP_WIFI_MAX_TX_POWER > 0
  GOTO_CHECK(esp_wifi_set_max_tx_power(CONFIG_APP_WIFI_MAX_TX_POWER), TAG, "Failed to set WiFi TX power", error);
#endif

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t reconnect_sta(const wifi_credentials_t *wifi_credentials)
{
  ESP_LOGI(TAG, "Reconnecting STA");

  // a disconnect here reports its own event, which must not hide the real failure
  if (sta_is_attempting)
  {
    sta_is_leaving = true;
    GOTO_CHECK(esp_wifi_disconnect(), TAG, "Failed to disconnect WiFi", error);
  }

  if (wifi_credentials != NULL)
  {
    esp_timer_stop(sta_retry_timer);
    sta_retry_count = 0;
    sta_last_reason = 0;
    sta_state = STA_STATE_CONNECTING;

    wifi_config_t wifi_config = {
      .sta =
        {
          .scan_method = WIFI_ALL_CHANNEL_SCAN,
          .failure_retry_cnt = CONFIG_APP_STA_MAXIMUM_RETRY,
        },
    };

    copy_ssid(wifi_config.sta.ssid, wifi_credentials->ssid);
    strlcpy((char *)wifi_config.sta.password, wifi_credentials->password, sizeof(wifi_config.sta.password));

    GOTO_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set STA config", error);
  }

  sta_is_attempting = true;
  GOTO_CHECK(esp_wifi_connect(), TAG, "Failed to connect WiFi", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

void get_sta_status(sta_status_t *status)
{
  wifi_config_t wifi_config = {0};
  esp_wifi_get_config(WIFI_IF_STA, &wifi_config);

  memset(status, 0, sizeof(sta_status_t));
  memcpy(status->ssid, wifi_config.sta.ssid, MAX_SSID_LEN);
  status->state = status->ssid[0] == '\0' ? STA_STATE_NO_CREDENTIALS : sta_state;
  status->reason = sta_last_reason;
  status->attempt = sta_retry_count + 1;
  status->drops = sta_drops;
  status->drop_reason = sta_last_drop_reason;

  if (sta_state != STA_STATE_CONNECTED || esp_wifi_sta_get_rssi(&status->rssi) != ESP_OK)
  {
    status->rssi = 0;
  }
}
