#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"

#include "app_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct
  {
    void (*on_sta_start)(void);
    void (*on_sta_got_ip)(void);
    void (*on_sta_lost_ip)(void);
  } wifi_event_handlers_t;

  esp_err_t init_network(const wifi_credentials_t *ap_credentials, const wifi_credentials_t *wifi_credentials);
  esp_err_t reconnect_sta(const wifi_credentials_t *wifi_credentials);

#ifdef __cplusplus
}
#endif

#endif // __APP_NETWORK_H__
