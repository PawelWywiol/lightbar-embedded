#ifndef __APP_EVENTS_H__
#define __APP_EVENTS_H__

#include "esp_event.h"

#ifdef __cplusplus
extern "C"
{
#endif

  ESP_EVENT_DECLARE_BASE(APP_EVENTS);

  enum
  {
    APP_EVENT_BASE = 0,
  };

#ifdef __cplusplus
}
#endif

#endif // __APP_EVENTS_H__
