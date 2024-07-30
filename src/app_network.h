#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "app_defines.h"
#include "app_nvs.h"

typedef struct
{
  char ssid[SIZE_WITH_TRAILING_ZERO(SSID_MAX_LENGTH)];
  char password[SIZE_WITH_TRAILING_ZERO(PASSWORD_MAX_LENGTH)];
} WiFiCredentials;

esp_err_t init_wifi();
esp_netif_t *init_ap(const WiFiCredentials *ap_credentials);
esp_err_t start_wifi();

void uid(char *uid, size_t length);

void reset_wifi_credentials(WiFiCredentials *wifi_credentials);
esp_err_t read_wifi_credentials(WiFiCredentials *wifi_credentials);
esp_err_t write_wifi_credentials(const WiFiCredentials *wifi_credentials);

void reset_ap_credentials(WiFiCredentials *wifi_credentials);
esp_err_t read_ap_credentials(WiFiCredentials *wifi_credentials);
esp_err_t write_ap_credentials(const WiFiCredentials *wifi_credentials);

#endif // __APP_NETWORK_H__
