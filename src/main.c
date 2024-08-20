#include "esp_log.h"

#include "app_defines.h"
#include "app_nvs.h"
#include "app_vfs.h"
#include "app_utils.h"
#include "app_network.h"
#include "app_server.h"

app_config_t app_config;

static const char *TAG = "APP_MAIN";

static esp_err_t app_api_post_handler(httpd_req_t *req)
{

  return ESP_OK;
}

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_credentials(&app_config));

  ESP_ERROR_CHECK(init_network());

  ESP_ERROR_CHECK(init_ap(&app_config.ap_credentials));
  ESP_ERROR_CHECK(start_wifi());

  ESP_ERROR_CHECK(init_server(&app_config));
}
