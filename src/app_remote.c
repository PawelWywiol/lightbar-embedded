#include "app_remote.h"

#if CONFIG_APP_REMOTE_ENABLED
#include "esp_event.h"
#include "esp_now.h"
#include <string.h>
#include <sys/param.h>

#include "app_defines.h"
#include "app_events.h"
#include "app_lights.h"

#define REMOTE_PACKET_LENGTH 13
#define REMOTE_PROGRAM_ON 0x91
#define REMOTE_PROGRAM_OTHER 0x81
#define REMOTE_BUTTON_DATA_TYPE 0x20
#define REMOTE_BATTERY_DATA_TYPE 0x01
#define REMOTE_HUE_STEP 4
#define REMOTE_HUE_RANGE (LIGHTS_ADJUST_HUE_MAX - LIGHTS_ADJUST_HUE_MIN + 1)

typedef struct __attribute__((packed)) remote_packet
{
  uint8_t program;
  uint32_t sequence;
  uint8_t button_type;
  uint8_t button;
  uint8_t battery_type;
  uint8_t battery;
  uint8_t reserved[4];
} remote_packet_t;

enum
{
  REMOTE_BUTTON_ON = 1,
  REMOTE_BUTTON_OFF = 2,
  REMOTE_BUTTON_MOON = 3,
  REMOTE_BUTTON_DIMMER = 8,
  REMOTE_BUTTON_BRIGHTER = 9,
  REMOTE_BUTTON_S1 = 16,
  REMOTE_BUTTON_S2 = 17,
  REMOTE_BUTTON_S3 = 18,
  REMOTE_BUTTON_S4 = 19,
};

static const char *TAG = "APP_REMOTE";
static const uint8_t speed_cycle[] = {100, 150, 200, 25, 50};

static void on_remote_receive(const esp_now_recv_info_t *info, const uint8_t *data, int length)
{
  static uint32_t last_sequence;
  static bool received;

  if (length != REMOTE_PACKET_LENGTH)
  {
    return;
  }

  remote_packet_t packet;
  memcpy(&packet, data, sizeof(packet));
  if ((packet.program != REMOTE_PROGRAM_ON && packet.program != REMOTE_PROGRAM_OTHER) ||
      packet.button_type != REMOTE_BUTTON_DATA_TYPE || packet.battery_type != REMOTE_BATTERY_DATA_TYPE ||
      (received && packet.sequence == last_sequence))
  {
    return;
  }

  received = true;
  last_sequence = packet.sequence;
  esp_event_post(APP_EVENTS, APP_EVENT_REMOTE_BUTTON, &packet.button, sizeof(packet.button), 0);
}

static uint8_t next_speed(uint8_t speed)
{
  for (size_t i = 0; i < sizeof(speed_cycle) - 1; i++)
  {
    if (speed_cycle[i] == speed)
    {
      return speed_cycle[i + 1];
    }
  }
  return LIGHTS_ADJUST_SPEED_DEFAULT;
}

static void change_adjust(uint8_t button)
{
  lights_adjust_t adjust = get_lights_adjust();

  switch (button)
  {
  case REMOTE_BUTTON_BRIGHTER:
    adjust.brightness = MIN(adjust.brightness + CONFIG_APP_REMOTE_BRIGHTNESS_STEP, LIGHTS_ADJUST_BRIGHTNESS_MAX);
    break;
  case REMOTE_BUTTON_DIMMER:
    adjust.brightness = MAX(adjust.brightness - CONFIG_APP_REMOTE_BRIGHTNESS_STEP, CONFIG_APP_REMOTE_BRIGHTNESS_MIN);
    break;
  case REMOTE_BUTTON_MOON:
    adjust.speed = next_speed(adjust.speed);
    break;
  case REMOTE_BUTTON_S2:
    adjust.hue = (adjust.hue - LIGHTS_ADJUST_HUE_MIN + REMOTE_HUE_STEP) % REMOTE_HUE_RANGE + LIGHTS_ADJUST_HUE_MIN;
    break;
  default:
    return;
  }

  set_lights_adjust(&adjust, sizeof(adjust));
}

static void on_remote_button(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  uint8_t button = *(uint8_t *)event_data;
  ESP_LOGI(TAG, "Button [%u]", button);

  if (button == REMOTE_BUTTON_ON)
  {
    set_lights_power(true);
    return;
  }

  if (!is_lights_power_on())
  {
    return;
  }

  switch (button)
  {
  case REMOTE_BUTTON_OFF:
    set_lights_power(false);
    break;
  case REMOTE_BUTTON_S1:
    esp_event_post(APP_EVENTS, APP_EVENT_PRESETS_AUTO, NULL, 0, 0);
    break;
  case REMOTE_BUTTON_S3:
    esp_event_post(APP_EVENTS, APP_EVENT_PRESETS_PREVIOUS, NULL, 0, 0);
    break;
  case REMOTE_BUTTON_S4:
    esp_event_post(APP_EVENTS, APP_EVENT_PRESETS_NEXT, NULL, 0, 0);
    break;
  default:
    change_adjust(button);
    break;
  }
}
#endif

esp_err_t init_remote(void)
{
#if CONFIG_APP_REMOTE_ENABLED
  GOTO_CHECK(esp_event_handler_instance_register(APP_EVENTS, APP_EVENT_REMOTE_BUTTON, on_remote_button, NULL, NULL),
             TAG, "Failed to register remote handler", error);
  GOTO_CHECK(esp_now_init(), TAG, "Failed to init ESP-NOW", error);
  GOTO_CHECK(esp_now_register_recv_cb(on_remote_receive), TAG, "Failed to register ESP-NOW receiver", error);

  return ESP_OK;
error:
  return ESP_FAIL;
#else
  return ESP_OK;
#endif
}
