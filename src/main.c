#include "app_defines.h"
#include "app_events.h"
#include "app_lights.h"
#include "app_network.h"
#include "app_nvs.h"
#include "app_server.h"
#include "app_utils.h"
#include "app_vfs.h"

wifi_credentials_t wifi_credentials;
wifi_credentials_t ap_credentials;

static const char *TAG = "APP_MAIN";

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static void process_request_wifi_credentials(void *data, size_t size)
{
  GOTO_CHECK(size != sizeof(wifi_credentials_t), TAG, "Invalid wifi credentials size", error);

  wifi_credentials_t *new_wifi_credentials = (wifi_credentials_t *)data;
  ESP_LOGI(TAG, "Processing wifi credentials : %s", new_wifi_credentials->ssid);

  GOTO_CHECK(strlen(new_wifi_credentials->ssid) == 0 || strlen(new_wifi_credentials->ssid) >= SSID_MAX_LENGTH ||
               strlen(new_wifi_credentials->password) == 0 ||
               strlen(new_wifi_credentials->password) >= PASSWORD_MAX_LENGTH,
             TAG, "Invalid wifi credentials", error);

  GOTO_CHECK(memcpy(&wifi_credentials, new_wifi_credentials, sizeof(wifi_credentials_t)) == NULL, TAG,
             "Failed to copy wifi credentials", error);

  GOTO_CHECK(write_wifi_credentials(&wifi_credentials), TAG, "Failed to write wifi credentials", error);

  GOTO_CHECK(reconnect_sta(new_wifi_credentials), TAG, "Failed to reconnect to wifi", error);

error:
}

static void app_event_process_request_chunks_file_handler(void *handler_args, esp_event_base_t base, int32_t id,
                                                          void *event_data)
{
  int available_frames = 0;
  int count_frames = 0;

  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  ESP_LOGI(TAG, "Processing request chunks file [%s]", chunk->uid);

  char path[FILE_SYSTEM_PATH_MAX_LENGTH] = {0};
  snprintf(path, FILE_SYSTEM_PATH_MAX_LENGTH, "%s/%s", FILE_SYSTEM_TEMP_BASE_PATH, chunk->uid);

  struct stat file_stat = {0};
  GOTO_CHECK(stat(path, &file_stat), TAG, "Failed to get file stat", error_unlink);
  GOTO_CHECK(file_stat.st_size != chunk->total, TAG, "File size mismatch", error_unlink);

  void *context = NULL;
  context = calloc(1, CONTEXT_BUFFER_MAX_LENGTH);
  GOTO_CHECK(context == NULL, TAG, "Failed to allocate memory for server context", error_unlink);

  int file = -1;
  file = open(path, O_RDONLY, 0);
  GOTO_CHECK(file == -1, TAG, "Failed to open file", error_free_context);

  do
  {
    connection_request_type_info_t chunk_end_info = CONNECTION_REQUEST_TYPE_NONE;
    connection_request_type_info_t chunk_type_info = CONNECTION_REQUEST_TYPE_NONE;

    size_t type_info_size = 0;
    type_info_size = read(file, &chunk_type_info, CONNECTION_REQUEST_TYPE_INFO_LENGTH);

    if (type_info_size == 0)
    {
      break;
    }

    count_frames++;

    GOTO_CHECK(type_info_size != CONNECTION_REQUEST_TYPE_INFO_LENGTH, TAG, "Failed to read request type",
               error_close_file);

    size_t chunk_context_size = 0;
    GOTO_CHECK(read(file, &chunk_context_size, CONNECTION_REQUEST_SIZE_INFO_LENGTH) !=
                 CONNECTION_REQUEST_SIZE_INFO_LENGTH,
               TAG, "Failed to read request size", error_free_context);

    if (chunk_context_size > CONTEXT_BUFFER_MAX_LENGTH)
    {
      GOTO_CHECK(lseek(file, chunk_context_size, SEEK_CUR) == -1, TAG, "Failed to skip request content",
                 error_close_file);
      GOTO_CHECK(read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) != CONNECTION_REQUEST_EOL_INFO_LENGTH,
                 TAG, "Failed to read request EOL", error_close_file);
      GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG, "Failed to read request EOL", error_close_file);
      continue;
    }

    GOTO_CHECK(read(file, context, chunk_context_size) != chunk_context_size, TAG, "Failed to read request content",
               error_close_file);
    GOTO_CHECK(read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) != CONNECTION_REQUEST_EOL_INFO_LENGTH,
               TAG, "Failed to read request EOL", error_close_file);
    GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG, "Failed to read request EOL", error_close_file);

    if (chunk_type_info == CONNECTION_REQUEST_FRAME_INFO)
    {
      available_frames++;
      continue;
    }

    if (chunk_type_info == CONNECTION_REQUEST_WIFI_INFO)
    {
      process_request_wifi_credentials(context, chunk_context_size);
    }

  } while (true);

error_close_file:
  close(file);
error_free_context:
  free(context);
error_unlink:
  if (count_frames > 0 && available_frames == count_frames)
  {
    ESP_LOGI(TAG, "All frames [%d] are available", count_frames);

    esp_event_post(APP_EVENTS, APP_EVENT_INIT_LIGHTS_SCHEMA, chunk, sizeof(request_chunk_data_t), portMAX_DELAY);
  }
  else if (unlink(path) == 0)
  {
    ESP_LOGI(TAG, "Removed file : %s", path);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to remove file : %s", path);
  }
}

static void app_event_process_request_chunk_handler(void *handler_args, esp_event_base_t base, int32_t id,
                                                    void *event_data)
{
  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  ESP_LOGI(TAG, "Processing request chunk [%d/%d]", chunk->processed, chunk->total);

  if (chunk->size <= 0)
  {
    ESP_LOGE(TAG, "Failed to read request content");
    return;
  }

  GOTO_CHECK(vfs_make_dir(FILE_SYSTEM_TEMP_BASE_PATH), TAG, "Failed to create temp directory", error);

  char path[FILE_SYSTEM_PATH_MAX_LENGTH];

  snprintf(path, FILE_SYSTEM_PATH_MAX_LENGTH, "%s/%s", FILE_SYSTEM_TEMP_BASE_PATH, chunk->uid);

  GOTO_CHECK(vfs_append_file(path, chunk->data, chunk->size), TAG, "Failed to append file", error);

  if (chunk->processed == chunk->total)
  {
    ESP_LOGI(TAG, "Received full file : %s", path);

    esp_event_post(APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNKS_FILE, chunk, sizeof(request_chunk_data_t),
                   portMAX_DELAY);
  }

error:
  return;
}

void app_main(void)
{
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNK,
                                                      app_event_process_request_chunk_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNKS_FILE,
                                                      app_event_process_request_chunks_file_handler, NULL, NULL));

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_wifi_credentials(&wifi_credentials));
  ESP_ERROR_CHECK(read_ap_credentials(&ap_credentials));

  ESP_ERROR_CHECK(init_network(&ap_credentials, &wifi_credentials));
  ESP_ERROR_CHECK(init_server(ap_credentials.ssid));

  ESP_ERROR_CHECK(init_lights_leds());
  ESP_ERROR_CHECK(init_lights_loop());
  ESP_ERROR_CHECK(init_lights_events());
}
