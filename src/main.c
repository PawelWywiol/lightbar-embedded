#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "app_defines.h"
#include "app_debug.h"
#include "app_file_system.h"
#include "app_network.h"

WiFiCredentials wifi_credentials;

static const char *TAG = "APP_MAIN";

void restart_app(void);

void app_main(void)
{
  vTaskDelay(APP_DELAY);

  ESP_LOGI(TAG, "Initializing application");

  if (!register_file_system())
  {
    restart_app();
    return;
  }

  createDirectory(APP_FILE_SYSTEM_CONFIG_DIRECTORY_PATH);

  if (read_wifi_credentials(&wifi_credentials) != ESP_OK)
  {
    restart_app();
    return;
  }

  ESP_LOGI(TAG, "Starting application");

  restart_app();
}

void restart_app(void)
{
  unregister_file_system();

  ESP_LOGI(TAG, "Restarting application");

  fflush(stdout);
  vTaskDelay(APP_DELAY);
  esp_restart();
}
