#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "app_defines.h"
#include "app_debug.h"
#include "app_file_system.h"

static const char *TAG = "APP_MAIN";

void restart_app(void)
{
  unregister_file_system();

  ESP_LOGI(TAG, "Restarting application");

  fflush(stdout);
  vTaskDelay(APP_DELAY);
  esp_restart();
}

void app_main(void)
{
  vTaskDelay(APP_DELAY);

  if (!register_file_system())
  {
    restart_app();
    return;
  }

  ESP_LOGI(TAG, "Starting application");

  restart_app();
}
