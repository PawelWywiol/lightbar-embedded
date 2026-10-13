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
static TaskHandle_t lights_loop_task;
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
  xTaskNotifyGive(lights_loop_task);
}

static void lights_loop_task_handler(void *arg)
{
  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    resolve_current_light_schema_frame();

    if (lights_data.status == LIGHTS_STATUS_RUNNING)
    {
      show_current_light_schema_frame();
    }
  }
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

  if (chunk->has_head && rename(lights_data.file_path, LIGHTS_CURRENT_FILE_PATH) == 0)
  {
    ESP_LOGI(TAG, "Saved lights schema [%s]", LIGHTS_CURRENT_FILE_PATH);
    return;
  }

  unlink(lights_data.file_path);
  return;

error_cleanup:
  unlink(lights_data.file_path);
  lights_data.status = LIGHTS_STATUS_NONE;
  lights_data.cached_frame_count = 0;

  ESP_LOGE(TAG, "Failed to process lights schema file [%s]", chunk->uid);
}

uint16_t calculate_frame_ticks(uint8_t tempo)
{
  if (tempo > LIGHTS_TEMPO_BPM_MAX)
  {
    return tempo < LIGHTS_TEMPO_RESERVED ? LIGHTS_TEMPO_RESERVED - tempo : 1;
  }

  uint32_t bpm = tempo ? tempo : 1;
  return (LIGHTS_TICKS_PER_MINUTE + bpm / 2) / bpm;
}

static size_t resolve_frame_header_size(connection_request_type_info_t chunk_type)
{
  return chunk_type == CONNECTION_REQUEST_FRAME_V2_INFO ? LIGHTS_FRAME_V2_HEADER_SIZE : LIGHTS_FRAME_HEADER_SIZE;
}

static size_t resolve_frame_colors_size(connection_request_type_info_t chunk_type, size_t chunk_context_size)
{
  size_t header_size = resolve_frame_header_size(chunk_type);
  if (chunk_context_size <= header_size)
  {
    return 0;
  }

  size_t colors_size = chunk_context_size - header_size;
  return colors_size < LIGHTS_FRAME_COLORS_MAX ? colors_size : LIGHTS_FRAME_COLORS_MAX;
}

esp_err_t resolve_lights_frame_from_context(frame_data_t *frame, uint8_t *colors,
                                            connection_request_type_info_t chunk_type, void *context,
                                            size_t chunk_context_size)
{
  size_t header_size = resolve_frame_header_size(chunk_type);
  GOTO_CHECK(chunk_context_size <= header_size, TAG, "Invalid frame context size", error);

  uint8_t *context_data = (uint8_t *)context;

  frame->flags = context_data[0];
  frame->hue_shift = (int8_t)(frame->flags & LIGHTS_FRAME_FLAG_HUE_SHIFT_MASK) >> LIGHTS_FRAME_FLAG_HUE_SHIFT_OFFSET;
  frame->pixel_shift = chunk_type == CONNECTION_REQUEST_FRAME_V2_INFO ? (int8_t)context_data[1] : 0;
  frame->tempo = context_data[header_size - 1];
  frame->duration_ticks = calculate_frame_ticks(frame->tempo);
  frame->colors_size = resolve_frame_colors_size(chunk_type, chunk_context_size);
  memcpy(colors, context_data + header_size, frame->colors_size);

  return ESP_OK;
error:
  return ESP_FAIL;
}

static uint32_t count_frames_in_file(int file, size_t *colors_total)
{
  uint32_t count = 0;
  *colors_total = 0;
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

    if (IS_CONNECTION_REQUEST_FRAME(chunk_type_info))
    {
      count++;
      *colors_total += resolve_frame_colors_size(chunk_type_info, chunk_context_size);
    }
  }

  return count;
}

static void shuffle_frame_color_indexes(uint8_t *indexes, uint8_t size)
{
  for (uint8_t i = 0; i < size; i++)
  {
    indexes[i] = i;
  }

  for (uint8_t i = size - 1; i > 0; i--)
  {
    uint8_t j = esp_random() % (i + 1);
    uint8_t index = indexes[i];
    indexes[i] = indexes[j];
    indexes[j] = index;
  }
}

static uint8_t add_hue_offset(uint8_t hue_offset, int8_t hue_shift)
{
  int hue = (hue_offset + hue_shift) % LIGHTS_PALLETTE_CHROMATIC_HUES;
  return hue < 0 ? hue + LIGHTS_PALLETTE_CHROMATIC_HUES : hue;
}

static uint8_t add_pixel_offset(uint8_t pixel_offset, int8_t pixel_shift, uint8_t colors_size)
{
  int pixel = (pixel_offset + pixel_shift) % colors_size;
  return pixel < 0 ? pixel + colors_size : pixel;
}

static uint8_t shift_color_hue(uint8_t color, uint8_t hue_offset)
{
  uint8_t hue = color & LIGHTS_PALLETTE_HUE_MASK;
  if (hue == LIGHTS_PALLETTE_HUE_MASK)
  {
    return color;
  }

  return (color & LIGHTS_PALLETTE_LIGHTNESS_MASK) | ((hue + hue_offset) % LIGHTS_PALLETTE_CHROMATIC_HUES);
}

static void resolve_frame_strip_colors(uint32_t frame_index, uint8_t hue_offset, uint8_t pixel_offset,
                                       uint8_t *strip_colors)
{
  const frame_data_t *frame = &lights_data.cached_frames[frame_index];
  const uint8_t *colors = lights_data.cached_colors + frame->colors_offset;
  uint8_t rotation = frame->colors_size - pixel_offset % frame->colors_size;

  if (!(frame->flags & LIGHTS_FRAME_FLAG_SHUFFLE))
  {
    for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
    {
      strip_colors[i] = shift_color_hue(colors[(i + rotation) % frame->colors_size], hue_offset);
    }
    return;
  }

  uint8_t indexes[LIGHTS_FRAME_COLORS_MAX];
  for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
  {
    uint8_t position = i % frame->colors_size;
    if (position == 0)
    {
      shuffle_frame_color_indexes(indexes, frame->colors_size);
    }
    strip_colors[i] = shift_color_hue(colors[(indexes[position] + rotation) % frame->colors_size], hue_offset);
  }
}

static void add_frame_offsets(const frame_data_t *frame)
{
  lights_data.hue_offset = add_hue_offset(lights_data.hue_offset, frame->hue_shift);
  lights_data.pixel_offset = add_pixel_offset(lights_data.pixel_offset, frame->pixel_shift, frame->colors_size);
}

static void resolve_next_strip_colors(void)
{
  uint32_t next_index = (lights_data.current_frame_index + 1) % lights_data.cached_frame_count;
  const frame_data_t *next_frame = &lights_data.cached_frames[next_index];
  resolve_frame_strip_colors(
    next_index, add_hue_offset(lights_data.hue_offset, next_frame->hue_shift),
    add_pixel_offset(lights_data.pixel_offset, next_frame->pixel_shift, next_frame->colors_size),
    lights_data.next_colors);
}

esp_err_t cache_light_schema_frames(void)
{
  free(lights_data.cached_frames);
  free(lights_data.cached_colors);
  lights_data.cached_frames = NULL;
  lights_data.cached_colors = NULL;
  lights_data.cached_frame_count = 0;
  lights_data.current_frame_index = 0;

  void *context = calloc(1, CONTEXT_BUFFER_MAX_LENGTH);
  GOTO_CHECK(context == NULL, TAG, "Failed to allocate context buffer", error);

  int file = open(lights_data.file_path, O_RDONLY, 0);
  GOTO_CHECK(file == -1, TAG, "Failed to open file", error_free_context);

  size_t colors_total = 0;
  uint32_t total_frames = count_frames_in_file(file, &colors_total);
  GOTO_CHECK(total_frames == 0, TAG, "No frames found", error_close_file);

  size_t frame_size = sizeof(frame_data_t);
  size_t free_heap = esp_get_free_heap_size();
  size_t reserved_heap = LIGHTS_MIN_FREE_HEAP + colors_total;
  uint32_t max_frames = (free_heap > reserved_heap) ? (free_heap - reserved_heap) / frame_size : 0;
  GOTO_CHECK(max_frames == 0, TAG, "Not enough heap for frames", error_close_file);

  uint32_t frames_to_cache = (total_frames < max_frames) ? total_frames : max_frames;

  lights_data.cached_frames = calloc(frames_to_cache, frame_size);
  GOTO_CHECK(lights_data.cached_frames == NULL, TAG, "Failed to allocate frame cache", error_close_file);

  lights_data.cached_colors = malloc(colors_total);
  GOTO_CHECK(lights_data.cached_colors == NULL, TAG, "Failed to allocate colors cache", error_free_cache);

  lseek(file, 0, SEEK_SET);
  uint32_t cached = 0;
  uint32_t colors_offset = 0;

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

    if (IS_CONNECTION_REQUEST_FRAME(chunk_type_info))
    {
      frame_data_t *frame = &lights_data.cached_frames[cached];
      frame->colors_offset = colors_offset;
      GOTO_CHECK(resolve_lights_frame_from_context(frame, lights_data.cached_colors + colors_offset, chunk_type_info,
                                                   context, chunk_context_size),
                 TAG, "Failed to resolve frame context", error_free_cache);
      colors_offset += frame->colors_size;
      cached++;
    }
  }

  close(file);
  free(context);

  GOTO_CHECK(cached == 0, TAG, "No frames cached", error_free_cache_only);

  lights_data.cached_frame_count = cached;
  lights_data.frames_count = cached;
  lights_data.current_frame_index = 0;
  lights_data.current_frame_tick = 0;
  lights_data.hue_offset = 0;
  lights_data.pixel_offset = 0;
  add_frame_offsets(&lights_data.cached_frames[0]);
  resolve_frame_strip_colors(0, lights_data.hue_offset, lights_data.pixel_offset, lights_data.current_colors);
  resolve_next_strip_colors();
  lights_data.status = LIGHTS_STATUS_RUNNING;

  ESP_LOGI(TAG, "Cached %lu frames in RAM", (unsigned long)cached);

  return ESP_OK;

error_free_cache:
  close(file);
  free(context);
error_free_cache_only:
  free(lights_data.cached_frames);
  free(lights_data.cached_colors);
  lights_data.cached_frames = NULL;
  lights_data.cached_colors = NULL;
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

  const frame_data_t *frame = &lights_data.cached_frames[lights_data.current_frame_index];
  if (++lights_data.current_frame_tick < frame->duration_ticks)
  {
    return ESP_OK;
  }

  lights_data.current_frame_index = (lights_data.current_frame_index + 1) % lights_data.cached_frame_count;
  lights_data.current_frame_tick = 0;
  add_frame_offsets(&lights_data.cached_frames[lights_data.current_frame_index]);
  memcpy(lights_data.current_colors, lights_data.next_colors, sizeof(lights_data.current_colors));
  resolve_next_strip_colors();

  return ESP_OK;
}

static void resolve_interpolated_color(uint8_t color_from, uint8_t color_to, float t, uint8_t *rgb)
{
  uint8_t rgb_from[3];
  uint8_t rgb_to[3];
  resolve_binary_color(color_from, rgb_from);
  resolve_binary_color(color_to, rgb_to);

  for (int c = 0; c < 3; c++)
  {
    int from = rgb_from[c];
    int to = rgb_to[c];
    int value = from + (int)((to - from) * t);
    if (value < 0)
    {
      value = 0;
    }
    if (value > 255)
    {
      value = 255;
    }
    rgb[c] = (uint8_t)value;
  }
}

static void set_led_strip_pixel(int index, const uint8_t *rgb)
{
  uint8_t *pixel = &led_strip_pixels[index * 3];
  pixel[LIGHTS_PIXEL_RED_INDEX] = rgb[0];
  pixel[LIGHTS_PIXEL_GREEN_INDEX] = rgb[1];
  pixel[LIGHTS_PIXEL_BLUE_INDEX] = rgb[2];
}

esp_err_t show_current_light_schema_frame(void)
{
  if (lights_data.status != LIGHTS_STATUS_RUNNING)
  {
    return ESP_FAIL;
  }

  if (lights_data.cached_frame_count == 0)
  {
    return ESP_FAIL;
  }

  const frame_data_t *frame = &lights_data.cached_frames[lights_data.current_frame_index];
  if (frame->flags & LIGHTS_FRAME_FLAG_FADE)
  {
    float t = (float)lights_data.current_frame_tick / (float)frame->duration_ticks;

    for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
    {
      uint8_t rgb[3];
      resolve_interpolated_color(lights_data.current_colors[i], lights_data.next_colors[i], t, rgb);
      set_led_strip_pixel(i, rgb);
    }
  }
  else
  {
    for (int i = 0; i < CONFIG_APP_LIGHTS_COUNT; i++)
    {
      uint8_t rgb[3];
      resolve_binary_color(lights_data.current_colors[i], rgb);
      set_led_strip_pixel(i, rgb);
    }
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
  GOTO_CHECK(xTaskCreate(lights_loop_task_handler, "lights_loop", LIGHTS_LOOP_TASK_STACK_SIZE, NULL,
                         LIGHTS_LOOP_TASK_PRIORITY, &lights_loop_task) != pdPASS,
             TAG, "Failed to create lights loop task", error);

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

  mkdir(LIGHTS_BASE_PATH, 0775);

  struct stat file_stat = {0};
  if (stat(LIGHTS_CURRENT_FILE_PATH, &file_stat) == 0)
  {
    strlcpy(lights_data.file_path, LIGHTS_CURRENT_FILE_PATH, sizeof(lights_data.file_path));

    if (cache_light_schema_frames() != ESP_OK)
    {
      ESP_LOGW(TAG, "Failed to load saved lights schema [%s]", LIGHTS_CURRENT_FILE_PATH);
    }
  }

  return ESP_OK;
error:
  return ESP_FAIL;
}
