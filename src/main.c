#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "app_defines.h"
#include "app_nvs.h"
#include "app_file_system.h"
#include "app_network.h"

WiFiCredentials wifi_credentials;
WiFiCredentials ap_credentials;

static const char *TAG = "APP_MAIN";

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(register_file_system());
  ESP_ERROR_CHECK(create_config_directory());
  ESP_ERROR_CHECK(read_wifi_credentials(&wifi_credentials));
  ESP_ERROR_CHECK(read_ap_credentials(&ap_credentials));

  ESP_LOGI(TAG, "Starting application");

  ESP_ERROR_CHECK(init_wifi());
  esp_netif_t *ap_netif = init_ap(&ap_credentials);
  ESP_ERROR_CHECK(start_wifi());
}
