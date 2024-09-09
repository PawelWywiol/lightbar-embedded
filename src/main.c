#include "app_defines.h"
#include "app_events.h"
#include "app_network.h"
#include "app_nvs.h"
#include "app_server.h"
#include "app_utils.h"
#include "app_vfs.h"

app_config_t app_config;

static const char *TAG = "APP_MAIN";

ESP_EVENT_DEFINE_BASE(APP_EVENTS);

static void app_event_process_request_chunks_file_handler(void *handler_args,
                                                          esp_event_base_t base,
                                                          int32_t id,
                                                          void *event_data) {
  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  ESP_LOGI(TAG, "Processing request chunks file [%s]", chunk->uid);

  char path[FILE_SYSTEM_PATH_MAX_LENGTH] = {0};
  snprintf(path, FILE_SYSTEM_PATH_MAX_LENGTH, "%s/%s",
           FILE_SYSTEM_TEMP_BASE_PATH, chunk->uid);

  struct stat file_stat = {0};
  GOTO_CHECK(stat(path, &file_stat), TAG, "Failed to get file stat",
             error_unlink);
  GOTO_CHECK(file_stat.st_size != chunk->total, TAG, "File size mismatch",
             error_unlink);

  void *context = NULL;
  context = calloc(1, CONTEXT_BUFFER_MAX_LENGTH);
  GOTO_CHECK(context == NULL, TAG,
             "Failed to allocate memory for server context", error_unlink);

  int file = -1;
  file = open(path, O_RDONLY, 0);
  GOTO_CHECK(file == -1, TAG, "Failed to open file", error_free_context);

  do {
    connection_request_type_info_t chunk_end_info =
        CONNECTION_REQUEST_TYPE_NONE;
    connection_request_type_info_t chunk_type_info =
        CONNECTION_REQUEST_TYPE_NONE;
    GOTO_CHECK(
        read(file, &chunk_type_info, CONNECTION_REQUEST_TYPE_INFO_LENGTH) !=
            CONNECTION_REQUEST_TYPE_INFO_LENGTH,
        TAG, "Failed to read request type", error_free_context);
    size_t chunk_context_size = 0;
    GOTO_CHECK(
        read(file, &chunk_context_size, CONNECTION_REQUEST_SIZE_INFO_LENGTH) !=
            CONNECTION_REQUEST_SIZE_INFO_LENGTH,
        TAG, "Failed to read request size", error_free_context);

    if (chunk_context_size > CONTEXT_BUFFER_MAX_LENGTH) {
      GOTO_CHECK(lseek(file, chunk_context_size, SEEK_CUR) == -1, TAG,
                 "Failed to skip request content", error_close_file);
      GOTO_CHECK(
          read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) !=
              CONNECTION_REQUEST_EOL_INFO_LENGTH,
          TAG, "Failed to read request EOL", error_close_file);
      GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG,
                 "Failed to read request EOL", error_close_file);
      continue;
    }

    GOTO_CHECK(read(file, context, chunk_context_size) != chunk_context_size,
               TAG, "Failed to read request content", error_close_file);
    GOTO_CHECK(
        read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) !=
            CONNECTION_REQUEST_EOL_INFO_LENGTH,
        TAG, "Failed to read request EOL", error_close_file);
    GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG,
               "Failed to read request EOL", error_close_file);

  } while (true);

  ESP_LOGI(TAG, "Processed request chunks file : %s", path);

error_close_file:
  close(file);
error_free_context:
  free(context);
error_unlink:
  if (unlink(path) == 0) {
    ESP_LOGI(TAG, "Removed file : %s", path);
  } else {
    ESP_LOGE(TAG, "Failed to remove file : %s", path);
  }

  return;
}

static void app_event_process_request_chunk_handler(void *handler_args,
                                                    esp_event_base_t base,
                                                    int32_t id,
                                                    void *event_data) {
  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  ESP_LOGI(TAG, "Processing request chunk [%d/%d]", chunk->processed,
           chunk->total);

  if (chunk->size <= 0) {
    ESP_LOGE(TAG, "Failed to read request content");
    return;
  }

  GOTO_CHECK(vfs_make_dir(FILE_SYSTEM_TEMP_BASE_PATH), TAG,
             "Failed to create temp directory", error);

  char path[FILE_SYSTEM_PATH_MAX_LENGTH];

  snprintf(path, FILE_SYSTEM_PATH_MAX_LENGTH, "%s/%s",
           FILE_SYSTEM_TEMP_BASE_PATH, chunk->uid);

  GOTO_CHECK(vfs_append_file(path, chunk->data, chunk->size), TAG,
             "Failed to append file", error);

  if (chunk->processed == chunk->total) {
    ESP_LOGI(TAG, "Received full file : %s", path);

    esp_event_post(APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNKS_FILE, chunk,
                   sizeof(request_chunk_data_t), portMAX_DELAY);
  }

error:
  return;
}

void app_main(void) {
  ESP_LOGI(TAG, "Initializing application");

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNK,
      app_event_process_request_chunk_handler, NULL, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNKS_FILE,
      app_event_process_request_chunks_file_handler, NULL, NULL));

  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_vfs());

  ESP_ERROR_CHECK(read_credentials(&app_config));

  ESP_ERROR_CHECK(init_network());

  ESP_ERROR_CHECK(init_ap(&app_config.ap_credentials));
  ESP_ERROR_CHECK(start_wifi());

  ESP_ERROR_CHECK(init_server(&app_config));
}
