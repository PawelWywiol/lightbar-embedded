#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include "esp_err.h"
#include "esp_log.h"

#include "app_defines.h"
#include "app_file_system.h"

typedef struct
{
  char ssid[SIZE_WITH_TRAILING_ZERO(SSID_MAX_LENGTH)];
  char password[SIZE_WITH_TRAILING_ZERO(PASSWORD_MAX_LENGTH)];
} WiFiCredentials;

void reset_wifi_credentials(WiFiCredentials *wifi_credentials);
esp_err_t read_wifi_credentials(WiFiCredentials *wifi_credentials);
esp_err_t write_wifi_credentials(const WiFiCredentials *wifi_credentials);

#endif // __APP_NETWORK_H__
