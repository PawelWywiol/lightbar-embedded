#include "esp_log.h"

#include "app_defines.h"
#include "app_nvs.h"
#include "app_vfs.h"
#include "app_network.h"
#include "app_server.h"

wifi_credentials_t wifi_credentials;
wifi_credentials_t ap_credentials;

static const char *TAG = "APP_MAIN";

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_wifi_credentials(&wifi_credentials));
  ESP_ERROR_CHECK(read_ap_credentials(&ap_credentials));

  ESP_ERROR_CHECK(init_netif());

  ESP_ERROR_CHECK(init_mdns());
  ESP_ERROR_CHECK(init_netbios());

  ESP_ERROR_CHECK(init_dhcps());

  ESP_ERROR_CHECK(init_wifi());
  ESP_ERROR_CHECK(init_ap(&ap_credentials));
  ESP_ERROR_CHECK(start_wifi());

  ESP_ERROR_CHECK(init_server());
}
