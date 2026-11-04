#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/inet.h"

#include "app_defines.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum sta_state
  {
    STA_STATE_NO_CREDENTIALS = 0,
    STA_STATE_CONNECTING = 1,
    STA_STATE_WAITING_FOR_IP = 2,
    STA_STATE_CONNECTED = 3,
  } sta_state_t;

  typedef struct sta_status
  {
    sta_state_t state;
    uint16_t reason;
    uint32_t attempt;
    uint32_t drops;
    uint16_t drop_reason;
    int rssi;
    char ssid[MAX_SSID_LEN + 1];
  } sta_status_t;

  esp_err_t init_network(const wifi_credentials_t *ap_credentials, const wifi_credentials_t *wifi_credentials);
  esp_err_t reconnect_sta(const wifi_credentials_t *wifi_credentials);
  void get_sta_status(sta_status_t *status);

#ifdef __cplusplus
}
#endif

#endif // __APP_NETWORK_H__
