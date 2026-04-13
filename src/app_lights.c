#include "app_lights.h"

static const char *TAG = "APP_LIGHTS";

static uint8_t led_strip_pixels[RMT_LED_NUMBERS * 3];

rmt_encoder_handle_t led_encoder = NULL;
led_strip_encoder_config_t encoder_config = {
  .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
};
rmt_channel_handle_t led_channel = NULL;
rmt_tx_channel_config_t tx_chan_config = {
  .clk_src = RMT_CLK_SRC_DEFAULT, //
  .gpio_num = RMT_LED_STRIP_GPIO_NUM,
  .mem_block_symbols = 64,
  .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
  .trans_queue_depth = 4,
};
rmt_transmit_config_t tx_config = {
  .loop_count = 0,
};
esp_timer_handle_t lights_loop_timer;
lights_data_t lights_data = {0};

uint32_t resolveColorHue(uint8_t color)
{
  return ((color & LIGHTS_PALLETTE_HUE_MASK) * 360) / LIGHTS_PALLETTE_HUE_MAX;
}

uint32_t resolveColorLightness(uint8_t color)
{
  return ((color & LIGHTS_PALLETTE_LIGHTNESS_MASK) >> 6) * LIGHTS_PALLETTE_LIGHTNESS_STEP +
         LIGHTS_PALLETTE_LIGHTNESS_BASE;
}

void resolve_binary_color(uint8_t color, uint8_t *rgb)
{
  uint32_t h = resolveColorHue(color);
  uint32_t s = 100;
  uint32_t v = resolveColorLightness(color);

  if (color % LIGHTS_PALLETTE_HUE_MAX == LIGHTS_PALLETTE_HUE_MASK)
  {
    s = 0;
  }

  if (color == LIGHTS_PALLETTE_HUE_MASK)
  {
    v = 0;
  }

  uint32_t rgb_max = v * 2.55f;
  uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

  uint32_t i = h / 60;
  uint32_t diff = h % 60;

  uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

  switch (i)
  {
  case 0:
    rgb[0] = rgb_max;
    rgb[1] = rgb_min + rgb_adj;
    rgb[2] = rgb_min;
    break;
  case 1:
    rgb[0] = rgb_max - rgb_adj;
    rgb[1] = rgb_max;
    rgb[2] = rgb_min;
    break;
  case 2:
    rgb[0] = rgb_min;
    rgb[1] = rgb_max;
    rgb[2] = rgb_min + rgb_adj;
    break;
  case 3:
    rgb[0] = rgb_min;
    rgb[1] = rgb_max - rgb_adj;
    rgb[2] = rgb_max;
    break;
  case 4:
    rgb[0] = rgb_min + rgb_adj;
    rgb[1] = rgb_min;
    rgb[2] = rgb_max;
    break;
  default:
    rgb[0] = rgb_max;
    rgb[1] = rgb_min;
    rgb[2] = rgb_max - rgb_adj;
    break;
  }
}

static void lights_loop_timer_callback(void *arg)
{
  resolve_current_light_schema_frame();

  if (lights_data.status != LIGHTS_STATUS_RUNNING)
  {
    return;
  }

  show_current_light_schema_frame();
}

static void on_init_light_schema_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  lights_data.status = LIGHTS_STATUS_STOPPED;

  request_chunk_data_t *chunk = (request_chunk_data_t *)event_data;

  ESP_LOGI(TAG, "Processing lights schema file [%s]", chunk->uid);

  snprintf(lights_data.file_path, FILE_SYSTEM_PATH_MAX_LENGTH, "%s/%s", FILE_SYSTEM_TEMP_BASE_PATH, chunk->uid);

  struct stat file_stat = {0};
  GOTO_CHECK(stat(lights_data.file_path, &file_stat), TAG, "Failed to get file stat", error_cleanup);
  GOTO_CHECK(file_stat.st_size != chunk->total, TAG, "File size mismatch", error_cleanup);

  chunk->total = file_stat.st_size;

  GOTO_CHECK(cache_light_schema_frames(), TAG, "Failed to cache lights schema frames", error_cleanup);

  unlink(lights_data.file_path);
  return;

error_cleanup:
  lights_data.status = LIGHTS_STATUS_NONE;
  lights_data.cached_frame_count = 0;

  ESP_LOGE(TAG, "Failed to process lights schema file [%s]", chunk->uid);
}

int64_t calculate_frame_duration(uint8_t tempo)
{
  return 60000000 / (tempo ? tempo : 1);
}

esp_err_t resolve_lights_frame_from_context(frame_data_t *frame, void *context, size_t chunk_context_size)
{
  GOTO_CHECK(chunk_context_size < LIGHTS_FRAME_MIN_CONTEXT_SIZE, TAG, "Invalid frame context size", error);

  uint8_t *context_data = (uint8_t *)context;
  uint8_t *context_colors_data = context_data + 2;
  size_t context_colors_size = chunk_context_size - 2;

  frame->type = context_data[0];
  frame->tempo = context_data[1];
  frame->duration = calculate_frame_duration(frame->tempo);

  for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
  {
    frame->colors[i] = context_colors_data[i % context_colors_size];
  }

  return ESP_OK;
error:
  return ESP_FAIL;
}

static uint32_t count_frames_in_file(int file)
{
  uint32_t count = 0;
  lseek(file, 0, SEEK_SET);

  while (true)
  {
    connection_request_type_info_t chunk_type_info = CONNECTION_REQUEST_TYPE_NONE;
    size_t type_info_size = read(file, &chunk_type_info, CONNECTION_REQUEST_TYPE_INFO_LENGTH);

    if (type_info_size == 0)
    {
      break;
    }

    if (type_info_size != CONNECTION_REQUEST_TYPE_INFO_LENGTH)
    {
      break;
    }

    size_t chunk_context_size = 0;
    if (read(file, &chunk_context_size, CONNECTION_REQUEST_SIZE_INFO_LENGTH) != CONNECTION_REQUEST_SIZE_INFO_LENGTH)
    {
      break;
    }

    if (lseek(file, chunk_context_size, SEEK_CUR) == -1)
    {
      break;
    }

    connection_request_type_info_t chunk_end_info = CONNECTION_REQUEST_TYPE_NONE;
    if (read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) != CONNECTION_REQUEST_EOL_INFO_LENGTH)
    {
      break;
    }

    if (chunk_end_info != CONNECTION_REQUEST_EOL_INFO)
    {
      break;
    }

    if (chunk_type_info == CONNECTION_REQUEST_FRAME_INFO)
    {
      count++;
    }
  }

  return count;
}

esp_err_t cache_light_schema_frames(void)
{
  free(lights_data.cached_frames);
  lights_data.cached_frames = NULL;
  lights_data.cached_frame_count = 0;
  lights_data.current_frame_index = 0;

  void *context = calloc(1, CONTEXT_BUFFER_MAX_LENGTH);
  GOTO_CHECK(context == NULL, TAG, "Failed to allocate context buffer", error);

  int file = open(lights_data.file_path, O_RDONLY, 0);
  GOTO_CHECK(file == -1, TAG, "Failed to open file", error_free_context);

  uint32_t total_frames = count_frames_in_file(file);
  GOTO_CHECK(total_frames == 0, TAG, "No frames found", error_close_file);

  size_t frame_size = sizeof(frame_data_t);
  size_t free_heap = esp_get_free_heap_size();
  uint32_t max_frames = (free_heap > LIGHTS_MIN_FREE_HEAP) ? (free_heap - LIGHTS_MIN_FREE_HEAP) / frame_size : 0;
  GOTO_CHECK(max_frames == 0, TAG, "Not enough heap for frames", error_close_file);

  uint32_t frames_to_cache = (total_frames < max_frames) ? total_frames : max_frames;

  lights_data.cached_frames = calloc(frames_to_cache, frame_size);
  GOTO_CHECK(lights_data.cached_frames == NULL, TAG, "Failed to allocate frame cache", error_close_file);

  lseek(file, 0, SEEK_SET);
  uint32_t cached = 0;

  while (cached < frames_to_cache)
  {
    connection_request_type_info_t chunk_type_info = CONNECTION_REQUEST_TYPE_NONE;
    connection_request_type_info_t chunk_end_info = CONNECTION_REQUEST_TYPE_NONE;

    size_t type_info_size = read(file, &chunk_type_info, CONNECTION_REQUEST_TYPE_INFO_LENGTH);
    if (type_info_size == 0)
    {
      break;
    }

    GOTO_CHECK(type_info_size != CONNECTION_REQUEST_TYPE_INFO_LENGTH, TAG, "Failed to read request type",
               error_free_cache);

    size_t chunk_context_size = 0;
    GOTO_CHECK(read(file, &chunk_context_size, CONNECTION_REQUEST_SIZE_INFO_LENGTH) !=
                 CONNECTION_REQUEST_SIZE_INFO_LENGTH,
               TAG, "Failed to read request size", error_free_cache);

    if (chunk_context_size > CONTEXT_BUFFER_MAX_LENGTH)
    {
      GOTO_CHECK(lseek(file, chunk_context_size, SEEK_CUR) == -1, TAG, "Failed to skip request content",
                 error_free_cache);
      GOTO_CHECK(read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) != CONNECTION_REQUEST_EOL_INFO_LENGTH,
                 TAG, "Failed to read request EOL", error_free_cache);
      GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG, "Failed to read request EOL", error_free_cache);
      continue;
    }

    GOTO_CHECK(read(file, context, chunk_context_size) != chunk_context_size, TAG, "Failed to read request content",
               error_free_cache);
    GOTO_CHECK(read(file, &chunk_end_info, CONNECTION_REQUEST_EOL_INFO_LENGTH) != CONNECTION_REQUEST_EOL_INFO_LENGTH,
               TAG, "Failed to read request EOL", error_free_cache);
    GOTO_CHECK(chunk_end_info != CONNECTION_REQUEST_EOL_INFO, TAG, "Failed to read request EOL", error_free_cache);

    if (chunk_type_info == CONNECTION_REQUEST_FRAME_INFO)
    {
      GOTO_CHECK(resolve_lights_frame_from_context(&lights_data.cached_frames[cached], context, chunk_context_size),
                 TAG, "Failed to resolve frame context", error_free_cache);
      lights_data.cached_frames[cached].chunk_type = chunk_type_info;
      cached++;
    }
  }

  close(file);
  free(context);

  GOTO_CHECK(cached == 0, TAG, "No frames cached", error_free_cache_only);

  lights_data.cached_frame_count = cached;
  lights_data.frames_count = cached;
  lights_data.current_frame_index = 0;
  lights_data.current_frame = lights_data.cached_frames[0];
  lights_data.current_frame.time = esp_timer_get_time();
  lights_data.next_frame_time = lights_data.current_frame.time + lights_data.current_frame.duration;
  lights_data.status = LIGHTS_STATUS_RUNNING;

  ESP_LOGI(TAG, "Cached %lu frames in RAM", (unsigned long)cached);

  return ESP_OK;

error_free_cache:
  close(file);
  free(context);
error_free_cache_only:
  free(lights_data.cached_frames);
  lights_data.cached_frames = NULL;
  lights_data.cached_frame_count = 0;
  return ESP_FAIL;
error_close_file:
  close(file);
error_free_context:
  free(context);
error:
  return ESP_FAIL;
}

esp_err_t resolve_current_light_schema_frame(void)
{
  if (lights_data.status != LIGHTS_STATUS_RUNNING || lights_data.cached_frame_count == 0)
  {
    return ESP_FAIL;
  }

  int64_t current_time = esp_timer_get_time();

  if (current_time < lights_data.next_frame_time)
  {
    return ESP_OK;
  }

  lights_data.current_frame_index = (lights_data.current_frame_index + 1) % lights_data.cached_frame_count;
  lights_data.current_frame = lights_data.cached_frames[lights_data.current_frame_index];
  lights_data.current_frame.time = current_time;
  lights_data.next_frame_time = current_time + lights_data.current_frame.duration;

  return ESP_OK;
}

esp_err_t show_current_light_schema_frame(void)
{
  if (lights_data.status != LIGHTS_STATUS_RUNNING)
  {
    return ESP_FAIL;
  }

  for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
  {
    resolve_binary_color(lights_data.current_frame.colors[i], &led_strip_pixels[i * 3]);
  }

  GOTO_CHECK(rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config), TAG,
             "Failed to transmit data", error);
  GOTO_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY), TAG, "Failed to wait for all data to be sent", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_lights_loop(void)
{
  ESP_LOGI(TAG, "Initializing lights loop");
  const esp_timer_create_args_t lights_loop_timer_args = {.callback = &lights_loop_timer_callback,
                                                          .name = "lights_loop_timer"};

  ESP_LOGI(TAG, "Create lights loop timer");
  GOTO_CHECK(esp_timer_create(&lights_loop_timer_args, &lights_loop_timer), TAG, "Failed to create timer", error);

  ESP_LOGI(TAG, "Start lights loop timer");
  GOTO_CHECK(esp_timer_start_periodic(lights_loop_timer, LIGHTS_LOOP_PERIOD), TAG, "Failed to start timer",
             error_cleanup);

  return ESP_OK;
error_cleanup:
  esp_timer_delete(lights_loop_timer);
error:
  return ESP_FAIL;
}

esp_err_t init_lights_leds(void)
{
  ESP_LOGI(TAG, "Initializing lights");

  ESP_LOGI(TAG, "Create RMT TX channel");
  GOTO_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_channel), TAG, "Failed to create RMT TX channel", error);

  ESP_LOGI(TAG, "Install led strip encoder");
  GOTO_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder), TAG, "Failed to install led strip encoder",
             error);

  ESP_LOGI(TAG, "Enable RMT TX channel");
  GOTO_CHECK(rmt_enable(led_channel), TAG, "Failed to enable RMT TX channel", error);

  ESP_LOGI(TAG, "Clear led strip pixels");
  memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

  ESP_LOGI(TAG, "Transmit led strip pixels");
  GOTO_CHECK(rmt_transmit(led_channel, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config), TAG,
             "Failed to transmit data", error);
  GOTO_CHECK(rmt_tx_wait_all_done(led_channel, portMAX_DELAY), TAG, "Failed to wait for all data to be sent", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t init_lights_events(void)
{
  ESP_LOGI(TAG, "Initializing lights events");

  GOTO_CHECK(esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_INIT_LIGHTS_SCHEMA, on_init_light_schema_handler,
                                                 NULL, NULL),
             TAG, "Failed to register lights schema file handler", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}
