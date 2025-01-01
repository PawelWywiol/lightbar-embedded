#include "app_server.h"

static const char *TAG = "APP_SERVER";

#define GZIP_EXTENSION ".gz"
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static char *_app_uid = NULL;
static char _host_ip[IP4_ADDR_STRLEN_MAX] = {0};

static void wifi_on_ip_change_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    memset((char *)_host_ip, 0, IP4_ADDR_STRLEN_MAX);
    snprintf((char *)_host_ip, IP4_ADDR_STRLEN_MAX, IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Got IP: %s", _host_ip);
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP)
  {
    memset((char *)_host_ip, 0, IP4_ADDR_STRLEN_MAX);
    ESP_LOGW(TAG, "Lost IP");
  }
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
  const char *type = "text/plain";

  if (CHECK_FILE_EXTENSION(filepath, ".html") || CHECK_FILE_EXTENSION(filepath, ".html.gz"))
  {
    type = "text/html";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".js") || CHECK_FILE_EXTENSION(filepath, ".js.gz"))
  {
    type = "application/javascript";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".css") || CHECK_FILE_EXTENSION(filepath, ".css.gz"))
  {
    type = "text/css";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".png") || CHECK_FILE_EXTENSION(filepath, ".png.gz"))
  {
    type = "image/png";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".jpg") || CHECK_FILE_EXTENSION(filepath, ".jpg.gz"))
  {
    type = "image/jpeg";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".ico") || CHECK_FILE_EXTENSION(filepath, ".ico.gz"))
  {
    type = "image/x-icon";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".svg") || CHECK_FILE_EXTENSION(filepath, ".svg.gz"))
  {
    type = "image/svg+xml";
  }
  else
  {
    return ESP_FAIL;
  }

  return httpd_resp_set_type(req, type);
}

static esp_err_t set_cors_headers(httpd_req_t *req)
{
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers",
                     "Content-Type, Access-Control-Allow-Headers, "
                     "Authorization, X-Requested-With");

  return ESP_OK;
}

static request_network_type_t get_request_network_type(httpd_req_t *req)
{
  uint32_t mask = 0x00FFFFFF;
  uint32_t gateway = inet_addr(CONFIG_APP_DHCP_IP_START) & mask;

  int socket = httpd_req_to_sockfd(req);
  char ipstr[INET6_ADDRSTRLEN];
  struct sockaddr_in6 socket_address;
  socklen_t socket_address_size = sizeof(socket_address);

  getpeername(socket, (struct sockaddr *)&socket_address, &socket_address_size);

  inet_ntop(AF_INET6, &socket_address.sin6_addr, ipstr, sizeof(ipstr));

  char *ipstr_colon = strrchr(ipstr, ':');

  if (ipstr_colon == NULL)
  {
    return NETWORK_TYPE_NONE;
  }

  uint32_t request_network_mask = (unsigned int)(inet_addr(ipstr_colon + 1) & mask);

  if (request_network_mask == gateway)
  {
    return NETWORK_TYPE_AP;
  }

  return NETWORK_TYPE_STA;
}

static esp_err_t common_get_handler(httpd_req_t *req)
{
  char filepath[FILE_SYSTEM_PATH_MAX_LENGTH] = FILE_SYSTEM_PUBLIC_BASE_PATH;

  if (req->uri[strlen(req->uri) - 1] == '/')
  {
    strlcat(filepath, "/index.html", sizeof(filepath));
  }
  else
  {
    strlcat(filepath, req->uri, sizeof(filepath));
  }

  snprintf(filepath, FILE_SYSTEM_PATH_MAX_LENGTH, "%s%s", clean_vfs_path(filepath), GZIP_EXTENSION);

  clean_vfs_path(filepath);

  int fd = open(filepath, O_RDONLY, 0);

  if (fd == -1)
  {
    ESP_LOGW(TAG, "Failed to open file : %s", filepath);
    filepath[strlen(filepath) - strlen(GZIP_EXTENSION)] = '\0';
    fd = open(filepath, O_RDONLY, 0);
  }
  else
  {
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  }

  if (fd == -1)
  {
    ESP_LOGE(TAG, "Failed to open file : %s", filepath);

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  set_cors_headers(req);

  if (set_content_type_from_file(req, filepath) != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to set content type for file : %s", filepath);
    close(fd);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set content type");
  }

  char *chunk = req->user_ctx;
  ssize_t read_bytes = 0;

  do
  {
    read_bytes = read(fd, chunk, CONTEXT_BUFFER_MAX_LENGTH);

    if (read_bytes == -1)
    {
      ESP_LOGE(TAG, "Failed to read file : %s", filepath);
    }
    else if (read_bytes > 0)
    {
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK)
      {
        ESP_LOGE(TAG, "File sending failed!");

        close(fd);
        httpd_resp_sendstr_chunk(req, NULL);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");

        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);

  ESP_LOGI(TAG, "File sending complete : %s", filepath);

  close(fd);
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t set_api_response(httpd_req_t *req, char *message)
{
  ESP_LOGI(TAG, "API GET request");

  set_cors_headers(req);
  httpd_resp_set_type(req, "application/json");

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "info");

  cJSON *data = cJSON_AddObjectToObject(root, "data");
  cJSON_AddNumberToObject(data, "leds", CONFIG_APP_LIGHTS_COUNT);

  vfs_size_t vfs_size = get_vfs_space_info();
  cJSON_AddNumberToObject(data, "free", vfs_size.free);
  cJSON_AddNumberToObject(data, "used", vfs_size.used);
  cJSON_AddNumberToObject(data, "total", vfs_size.total);

  if (message != NULL)
  {
    cJSON_AddStringToObject(root, "message", message);
    httpd_resp_set_status(req, HTTPD_400);
  }

  if (_app_uid != NULL)
  {
    cJSON_AddStringToObject(data, "uid", _app_uid);
  }

  request_network_type_t network_type = get_request_network_type(req);
  cJSON_AddNumberToObject(data, "network", network_type);

  if (network_type == NETWORK_TYPE_AP)
  {
    cJSON_AddStringToObject(data, "host", _host_ip);
  }

  const char *info = cJSON_PrintUnformatted(root);

  httpd_resp_sendstr(req, info);
  free((void *)info);

  cJSON_Delete(root);
  return ESP_OK;
}

static esp_err_t api_get_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "API GET request");

  esp_event_post(APP_EVENTS, APP_EVENT_BASE, NULL, 0, portMAX_DELAY);

  return set_api_response(req, NULL);
}

static esp_err_t api_post_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "API POST request");

  vfs_size_t vfs_size = get_vfs_space_info();
  size_t content_length = req->content_len;

  if (content_length >= vfs_size.free)
  {
    ESP_LOGE(TAG, "Request content length is too large");
    return set_api_response(req, "Request content length is too large");
  }

  request_chunk_data_t chunk = {.data = req->user_ctx, .total = content_length, .size = 0, .processed = 0};

  snprintf(chunk.uid, UID_MAX_LENGTH, "%08x", (unsigned int)esp_timer_get_time());
  uid(chunk.uid + strlen(chunk.uid), UID_MAX_LENGTH - strlen(chunk.uid));
  chunk.uid[UID_MAX_LENGTH - 1] = '\0';

  do
  {
    chunk.size = httpd_req_recv(req, req->user_ctx, content_length);
    chunk.processed += chunk.size;

    if (chunk.size <= 0)
    {
      ESP_LOGE(TAG, "Failed to read request content");
      return set_api_response(req, "Failed to read request content");
    }

    esp_event_post(APP_EVENTS, APP_EVENT_PROCESS_REQUEST_CHUNK, &chunk, sizeof(request_chunk_data_t), portMAX_DELAY);

  } while (chunk.size > 0 && chunk.processed < content_length);

  return set_api_response(req, NULL);
}

esp_err_t init_server(char *app_uid)
{
  ESP_LOGI(TAG, "Initializing server");

  _app_uid = app_uid;

  void *context = calloc(1, CONTEXT_BUFFER_MAX_LENGTH);
  GOTO_CHECK(context == NULL, TAG, "Failed to allocate memory for server context", error);

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  GOTO_CHECK(httpd_start(&server, &config), TAG, "Failed to start server", error_free_context);

  httpd_uri_t api_get_uri = {.uri = "/api", .method = HTTP_GET, .handler = api_get_handler, .user_ctx = context};
  httpd_register_uri_handler(server, &api_get_uri);

  httpd_uri_t api_post_uri = {.uri = "/api", .method = HTTP_POST, .handler = api_post_handler, .user_ctx = context};
  httpd_register_uri_handler(server, &api_post_uri);

  httpd_uri_t common_get_uri = {.uri = "/*", .method = HTTP_GET, .handler = common_get_handler, .user_ctx = context};
  httpd_register_uri_handler(server, &common_get_uri);

  GOTO_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_on_ip_change_handler, NULL, NULL),
             TAG, "Failed to register WiFi event handler", error);

  return ESP_OK;
error_free_context:
  free(context);
error:
  return ESP_FAIL;
}
