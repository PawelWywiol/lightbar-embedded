#include "app_defines.h"
#include "app_events.h"
#include "app_nvs.h"
#include "app_vfs.h"
#include "app_utils.h"
#include "app_network.h"
#include "app_server.h"

app_config_t app_config;

static const char *TAG = "APP_MAIN";

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static void app_event_post_chunk_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  connection_request_type_info_t type_info = *((connection_request_type_info_t *)chunk->data);
  size_t data_size = *((size_t *)(chunk->data + CONNECTION_REQUEST_TYPE_INFO_LENGTH));

  if (chunk->size != data_size + CONNECTION_REQUEST_INFO_LENGTH)
  {
    ESP_LOGE(TAG, "APP_EVENT_POST_CHUNK: Invalid chunk size [%d][%d]", chunk->size, data_size);
    return;
  }

  connection_request_type_info_t eol_info =
      *((connection_request_type_info_t *)(chunk->data +
                                           CONNECTION_REQUEST_TYPE_INFO_LENGTH +
                                           CONNECTION_REQUEST_SIZE_INFO_LENGTH +
                                           data_size));

  if (eol_info != CONNECTION_REQUEST_EOL_INFO)
  {
    ESP_LOGE(TAG, "APP_EVENT_POST_CHUNK: Invalid EOL info [%04x]", eol_info);
    return;
  }

  ESP_LOGI(TAG, "APP_EVENT_POST_CHUNK: [%04x][%d][%s]", type_info, data_size, chunk->uid);
}

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_POST_CHUNK, app_event_post_chunk_handler, NULL, NULL));

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_credentials(&app_config));

  ESP_ERROR_CHECK(init_network());

  ESP_ERROR_CHECK(init_ap(&app_config.ap_credentials));
  ESP_ERROR_CHECK(start_wifi());

  ESP_ERROR_CHECK(init_server(&app_config));
}
