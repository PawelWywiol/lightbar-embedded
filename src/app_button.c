#include "app_button.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#include "app_defines.h"
#include "app_events.h"

#define BUTTON_POLL_MS 20
#define BUTTON_MIN_PRESS_POLLS 2
#define BUTTON_HOLD_POLLS (3000 / BUTTON_POLL_MS)

#if CONFIG_APP_BUTTON_GPIO >= 0
static const char *TAG = "APP_BUTTON";

static void on_button_poll(void *arg)
{
  static uint32_t pressed_polls = 0;

  if (gpio_get_level(CONFIG_APP_BUTTON_GPIO) == 0)
  {
    if (++pressed_polls == BUTTON_HOLD_POLLS)
    {
      esp_event_post(APP_EVENTS, APP_EVENT_PRESETS_AUTO, NULL, 0, 0);
    }
    return;
  }

  if (pressed_polls >= BUTTON_MIN_PRESS_POLLS && pressed_polls < BUTTON_HOLD_POLLS)
  {
    esp_event_post(APP_EVENTS, APP_EVENT_PRESETS_NEXT, NULL, 0, 0);
  }
  pressed_polls = 0;
}
#endif

esp_err_t init_button(void)
{
#if CONFIG_APP_BUTTON_GPIO >= 0
  const gpio_config_t config = {
    .pin_bit_mask = 1ULL << CONFIG_APP_BUTTON_GPIO,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
  };
  GOTO_CHECK(gpio_config(&config), TAG, "Failed to configure button GPIO", error);

  const esp_timer_create_args_t timer_args = {.callback = &on_button_poll, .name = "button_poll"};
  esp_timer_handle_t timer;
  GOTO_CHECK(esp_timer_create(&timer_args, &timer), TAG, "Failed to create button timer", error);
  GOTO_CHECK(esp_timer_start_periodic(timer, BUTTON_POLL_MS * 1000), TAG, "Failed to start button timer", error);

  return ESP_OK;
error:
  return ESP_FAIL;
#else
  return ESP_OK;
#endif
}
