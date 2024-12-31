#ifndef __APP_LIGHTS_H__
#define __APP_LIGHTS_H__

#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
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

#define LIGHTS_LOOP_FPS 24
#define LIGHTS_LOOP_PERIOD (1000000 / LIGHTS_LOOP_FPS)
#define LIGHTS_FRAME_MIN_CONTEXT_SIZE 3

#define LIGHTS_PALLETTE_HUE_MASK 0b00111111
#define LIGHTS_PALLETTE_HUE_MAX (LIGHTS_PALLETTE_HUE_MASK + 1)
#define LIGHTS_PALLETTE_HUE_MAX_VALUE 360
#define LIGHTS_PALLETTE_LIGHTNESS_MASK 0b11000000
#define LIGHTS_PALLETTE_LIGHTNESS_MAX ((LIGHTS_PALLETTE_LIGHTNESS_MASK >> 6) + 1)
#define LIGHTS_PALLETTE_LIGHTNESS_STEP 30
#define LIGHTS_PALLETTE_LIGHTNESS_BASE 10

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM CONFIG_APP_LIGHTS_GPIO
#define RMT_LED_NUMBERS CONFIG_APP_LIGHTS_COUNT

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
    connection_request_type_info_t chunk_type;
    uint8_t type;
    uint8_t tempo;
    uint8_t colors[CONFIG_APP_LIGHTS_COUNT];
    ssize_t offset;
    int64_t time;
    int64_t duration;
  } frame_data_t;

  typedef struct lights_data
  {
    char file_path[FILE_SYSTEM_PATH_MAX_LENGTH];
    ssize_t file_size;
    u_int32_t frames_count;
    frame_data_t first_frame;
    frame_data_t current_frame;
    frame_data_t next_frame;
    lights_status_t status;
  } lights_data_t;

  int64_t calculate_frame_duration(uint8_t tempo);
  esp_err_t resolve_lights_frame_from_context(frame_data_t *frame, void *context, size_t chunk_context_size);
  esp_err_t process_current_light_schema_file(void);
  esp_err_t resolve_current_light_schema_frame(void);
  esp_err_t show_current_light_schema_frame(void);

  esp_err_t init_lights_leds(void);
  esp_err_t init_lights_loop(void);
  esp_err_t init_lights_events(void);

#ifdef __cplusplus
}
#endif

#endif // __APP_LIGHTS_H__
