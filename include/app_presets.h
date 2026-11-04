#ifndef __APP_PRESETS_H__
#define __APP_PRESETS_H__

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  esp_err_t init_presets(void);
  bool restore_preset(void);
  void presets_on_lights_request(bool saved);

#ifdef __cplusplus
}
#endif

#endif // __APP_PRESETS_H__
