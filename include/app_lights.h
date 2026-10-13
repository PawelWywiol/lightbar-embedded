#ifndef __APP_LIGHTS_H__
#define __APP_LIGHTS_H__

#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_defines.h"
#include "app_events.h"

#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LIGHTS_LOOP_FPS 60
#define LIGHTS_LOOP_PERIOD (1000000 / LIGHTS_LOOP_FPS)
#define LIGHTS_LOOP_TASK_STACK_SIZE 3072
#define LIGHTS_LOOP_TASK_PRIORITY 10
#define LIGHTS_TICKS_PER_MINUTE (LIGHTS_LOOP_FPS * 60)
#define LIGHTS_TEMPO_BPM_MAX 240
#define LIGHTS_TEMPO_RESERVED 255
#define LIGHTS_FRAME_HEADER_SIZE 2
#define LIGHTS_FRAME_V2_HEADER_SIZE 3
#define LIGHTS_FRAME_COLORS_MAX 255
#define LIGHTS_FRAME_FLAG_FADE (1 << 0)
#define LIGHTS_FRAME_FLAG_SHUFFLE (1 << 1)
#define LIGHTS_FRAME_FLAG_HUE_SHIFT_MASK 0b11111100
#define LIGHTS_FRAME_FLAG_HUE_SHIFT_OFFSET 2
#define LIGHTS_BASE_PATH FILE_SYSTEM_BASE_PATH "/lights"
#define LIGHTS_CURRENT_FILE_PATH LIGHTS_BASE_PATH "/current"

#define LIGHTS_PALLETTE_HUE_MASK 0b00111111
#define LIGHTS_PALLETTE_HUE_MAX (LIGHTS_PALLETTE_HUE_MASK + 1)
#define LIGHTS_PALLETTE_CHROMATIC_HUES LIGHTS_PALLETTE_HUE_MASK
#define LIGHTS_PALLETTE_HUE_MAX_VALUE 360
#define LIGHTS_PALLETTE_LIGHTNESS_MASK 0b11000000
#define LIGHTS_PALLETTE_LIGHTNESS_MAX ((LIGHTS_PALLETTE_LIGHTNESS_MASK >> 6) + 1)
#define LIGHTS_PALLETTE_LIGHTNESS_STEP 30
#define LIGHTS_PALLETTE_LIGHTNESS_BASE 10

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM CONFIG_APP_LIGHTS_GPIO
#define RMT_LED_NUMBERS CONFIG_APP_LIGHTS_COUNT

#if CONFIG_APP_LIGHTS_COLOR_ORDER_RGB
#define LIGHTS_PIXEL_RED_INDEX 0
#define LIGHTS_PIXEL_GREEN_INDEX 1
#else
#define LIGHTS_PIXEL_RED_INDEX 1
#define LIGHTS_PIXEL_GREEN_INDEX 0
#endif
#define LIGHTS_PIXEL_BLUE_INDEX 2

  typedef enum lights_status
  {
    LIGHTS_STATUS_NONE = 0,
    LIGHTS_STATUS_PROCESSING,
    LIGHTS_STATUS_STOPPED,
    LIGHTS_STATUS_RUNNING,
    LIGHTS_STATUS_PAUSED
  } lights_status_t;

  typedef struct frame_data
  {
    uint8_t flags;
    uint8_t tempo;
    uint8_t colors_size;
    int8_t hue_shift;
    int8_t pixel_shift;
    uint16_t duration_ticks;
    uint32_t colors_offset;
  } frame_data_t;

#define LIGHTS_MIN_FREE_HEAP 32768

  typedef struct lights_data
  {
    char file_path[FILE_SYSTEM_PATH_MAX_LENGTH];
    ssize_t file_size;
    uint32_t frames_count;
    frame_data_t *cached_frames;
    uint8_t *cached_colors;
    uint32_t cached_frame_count;
    uint32_t current_frame_index;
    uint8_t current_colors[CONFIG_APP_LIGHTS_COUNT];
    uint8_t next_colors[CONFIG_APP_LIGHTS_COUNT];
    uint8_t hue_offset;
    uint8_t pixel_offset;
    uint16_t current_frame_tick;
    lights_status_t status;
  } lights_data_t;

  uint16_t calculate_frame_ticks(uint8_t tempo);
  esp_err_t resolve_lights_frame_from_context(frame_data_t *frame, uint8_t *colors,
                                              connection_request_type_info_t chunk_type, void *context,
                                              size_t chunk_context_size);
  esp_err_t cache_light_schema_frames(void);
  esp_err_t resolve_current_light_schema_frame(void);
  esp_err_t show_current_light_schema_frame(void);

  esp_err_t init_lights_leds(void);
  esp_err_t init_lights_loop(void);
  esp_err_t init_lights_events(void);

#ifdef __cplusplus
}
#endif

#endif // __APP_LIGHTS_H__
