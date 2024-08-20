#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/inet.h"

#include "app_defines.h"

esp_err_t init_netif();
esp_err_t init_wifi();
esp_err_t init_ap(const wifi_credentials_t *ap_credentials);
esp_err_t start_wifi();

esp_err_t init_mdns();
esp_err_t init_netbios();

esp_err_t init_dhcps();

#endif // __APP_NETWORK_H__
