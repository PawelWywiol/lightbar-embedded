#include "app_presets.h"

#include "esp_event.h"
#include "esp_timer.h"
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_defines.h"
#include "app_events.h"
#include "app_lights.h"
#include "app_nvs.h"

#define PRESETS_BASE_PATH FILE_SYSTEM_BASE_PATH "/presets"
#define PRESETS_NAME_MAX_LENGTH 64
#define PRESETS_SAVE_DELAY_US (3 * 1000 * 1000)
#define PRESETS_AUTO_INTERVAL_US (CONFIG_APP_PRESETS_AUTO_INTERVAL * 1000000ULL)

typedef struct presets_state
{
  char name[PRESETS_NAME_MAX_LENGTH];
  bool auto_cycle;
} presets_state_t;

static const char *TAG = "APP_PRESETS";

static char last_preset[PRESETS_NAME_MAX_LENGTH];
static presets_state_t state;
static presets_state_t saved_state;
static esp_timer_handle_t auto_timer;
static esp_timer_handle_t save_timer;

// the system event task has a small stack, so path and name buffers stay static
static bool find_next_preset(const char *after, char *next)
{
  static char first[PRESETS_NAME_MAX_LENGTH];
  static char greater[PRESETS_NAME_MAX_LENGTH];
  first[0] = '\0';
  greater[0] = '\0';

  DIR *dir = opendir(PRESETS_BASE_PATH);
  if (dir == NULL)
  {
    return false;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    const char *name = entry->d_name;
    if (entry->d_type != DT_REG || strlen(name) >= PRESETS_NAME_MAX_LENGTH)
    {
      continue;
    }
    if (first[0] == '\0' || strcmp(name, first) < 0)
    {
      strlcpy(first, name, sizeof(first));
    }
    if (strcmp(name, after) > 0 && (greater[0] == '\0' || strcmp(name, greater) < 0))
    {
      strlcpy(greater, name, sizeof(greater));
    }
  }
  closedir(dir);

  if (first[0] == '\0')
  {
    return false;
  }

  strlcpy(next, greater[0] != '\0' ? greater : first, PRESETS_NAME_MAX_LENGTH);
  return true;
}

static esp_err_t load_preset(const char *name)
{
  static char path[FILE_SYSTEM_PATH_MAX_LENGTH];
  snprintf(path, sizeof(path), "%s/%s", PRESETS_BASE_PATH, name);
  return load_light_schema_file(path);
}

static bool play_next_preset(void)
{
  static char candidate[PRESETS_NAME_MAX_LENGTH];
  static char first_tried[PRESETS_NAME_MAX_LENGTH];
  strlcpy(candidate, last_preset, sizeof(candidate));
  first_tried[0] = '\0';

  while (find_next_preset(candidate, candidate))
  {
    if (strcmp(candidate, first_tried) == 0)
    {
      break;
    }
    if (first_tried[0] == '\0')
    {
      strlcpy(first_tried, candidate, sizeof(first_tried));
    }
    if (load_preset(candidate) == ESP_OK)
    {
      strlcpy(last_preset, candidate, sizeof(last_preset));
      return true;
    }
    ESP_LOGW(TAG, "Skipped preset [%s]", candidate);
  }

  ESP_LOGW(TAG, "No preset to play");
  return false;
}

static void schedule_save(void)
{
  if (esp_timer_is_active(save_timer))
  {
    esp_timer_restart(save_timer, PRESETS_SAVE_DELAY_US);
    return;
  }
  esp_timer_start_once(save_timer, PRESETS_SAVE_DELAY_US);
}

static void stop_auto(void)
{
  if (esp_timer_is_active(auto_timer))
  {
    esp_timer_stop(auto_timer);
  }
}

static void start_auto(void)
{
  stop_auto();
  esp_timer_start_periodic(auto_timer, PRESETS_AUTO_INTERVAL_US);
}

static void remember_user_choice(bool auto_cycle)
{
  memset(&state, 0, sizeof(state));
  strlcpy(state.name, last_preset, sizeof(state.name));
  state.auto_cycle = auto_cycle;
  schedule_save();
}

void presets_on_lights_request(bool saved)
{
  stop_auto();

  if (saved)
  {
    memset(&state, 0, sizeof(state));
  }
  else
  {
    state.auto_cycle = false;
  }
  schedule_save();
}

static void on_presets_event(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  switch (id)
  {
  case APP_EVENT_BUTTON_CLICK:
    next_lights_request_sequence();
    stop_auto();
    play_next_preset();
    remember_user_choice(false);
    break;
  case APP_EVENT_BUTTON_HOLD:
    next_lights_request_sequence();
    play_next_preset();
    start_auto();
    remember_user_choice(true);
    break;
  case APP_EVENT_PRESETS_AUTO_STEP:
    if (state.auto_cycle)
    {
      play_next_preset();
    }
    break;
  case APP_EVENT_SAVE_PRESETS_STATE:
    if (memcmp(&state, &saved_state, sizeof(state)) != 0 &&
        nvs_write_data(CONFIG_APP_NVS_PRESETS_STATE_KEY, &state, sizeof(state)) == ESP_OK)
    {
      saved_state = state;
    }
    break;
  default:
    break;
  }
}

static void post_timer_event(void *arg)
{
  esp_event_post(APP_EVENTS, (int32_t)(intptr_t)arg, NULL, 0, 0);
}

esp_err_t init_presets(void)
{
  const int32_t events[] = {APP_EVENT_BUTTON_CLICK, APP_EVENT_BUTTON_HOLD, APP_EVENT_PRESETS_AUTO_STEP,
                            APP_EVENT_SAVE_PRESETS_STATE};
  for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); i++)
  {
    GOTO_CHECK(esp_event_handler_instance_register(APP_EVENTS, events[i], on_presets_event, NULL, NULL), TAG,
               "Failed to register presets handler", error);
  }

  const esp_timer_create_args_t auto_args = {
    .callback = &post_timer_event, .arg = (void *)APP_EVENT_PRESETS_AUTO_STEP, .name = "presets_auto"};
  GOTO_CHECK(esp_timer_create(&auto_args, &auto_timer), TAG, "Failed to create presets auto timer", error);

  const esp_timer_create_args_t save_args = {
    .callback = &post_timer_event, .arg = (void *)APP_EVENT_SAVE_PRESETS_STATE, .name = "presets_save"};
  GOTO_CHECK(esp_timer_create(&save_args, &save_timer), TAG, "Failed to create presets save timer", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

bool restore_preset(void)
{
  if (CONFIG_APP_BUTTON_GPIO < 0 ||
      nvs_read_data(CONFIG_APP_NVS_PRESETS_STATE_KEY, &saved_state, sizeof(saved_state)) != ESP_OK)
  {
    memset(&saved_state, 0, sizeof(saved_state));
    return false;
  }

  saved_state.name[PRESETS_NAME_MAX_LENGTH - 1] = '\0';
  state = saved_state;
  strlcpy(last_preset, state.name, sizeof(last_preset));

  if (state.auto_cycle)
  {
    start_auto();
  }

  if (state.name[0] == '\0')
  {
    return false;
  }

  return load_preset(state.name) == ESP_OK || play_next_preset();
}
