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
#include "lwip/apps/netbiosns.h"
#include "lwip/inet.h"
#include "mdns.h"

#include "app_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t init_network();
esp_err_t init_ap(const wifi_credentials_t *ap_credentials);
esp_err_t start_wifi();

#ifdef __cplusplus
}
#endif

#endif // __APP_NETWORK_H__
