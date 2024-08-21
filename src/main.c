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
  ESP_LOGI(TAG, "API POST request");

  size_t content_length = req->content_len;
  if (content_length >= SERVER_CONTEXT_BUFFER_MAX_LENGTH)
  {
    ESP_LOGE(TAG, "Request content length is too large");
    return set_api_response(req, "Request content length is too large");
  }

  ssize_t read_bytes = httpd_req_recv(req, req->user_ctx, content_length);

  if (read_bytes <= 0)
  {
    ESP_LOGE(TAG, "Failed to read request content");
    return set_api_response(req, "Failed to read request content");
  }

  ((char *)req->user_ctx)[read_bytes] = '\0';

  ESP_LOGI(TAG, "Request content : %s", (char *)req->user_ctx);

  return ESP_OK;
}

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  app_config.app_api_post_handler = app_api_post_handler;

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_credentials(&app_config));

  ESP_ERROR_CHECK(init_network());

  ESP_ERROR_CHECK(init_ap(&app_config.ap_credentials));
  ESP_ERROR_CHECK(start_wifi());

  ESP_ERROR_CHECK(init_server(&app_config));
}
