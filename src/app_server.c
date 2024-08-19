#include "app_server.h"

static const char *TAG = "APP_SERVER";

typedef struct server_context
{
  char base_path[SIZE_WITH_TRAILING_ZERO(FILE_SYSTEM_BASE_PATH_MAX_LENGTH)];
  char scratch[SERVER_CONTEXT_BUFFER_MAX_LENGTH];
} server_context_t;

#define GZIP_EXTENSION ".gz"
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

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
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With");

  return ESP_OK;
}

static esp_err_t common_get_handler(httpd_req_t *req)
{
  char filepath[FILE_SYSTEM_PATH_MAX_LENGTH];

  server_context_t *context = (server_context_t *)req->user_ctx;
  strlcpy(filepath, context->base_path, sizeof(filepath));

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

  char *chunk = context->scratch;
  ssize_t read_bytes;
  do
  {
    read_bytes = read(fd, chunk, SERVER_CONTEXT_BUFFER_MAX_LENGTH);
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

  ESP_LOGI(TAG, "File sending complete");

  close(fd);
  httpd_resp_send_chunk(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t api_get_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "API GET request");

  set_cors_headers(req);
  httpd_resp_set_type(req, "application/json");

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "info");
  cJSON_AddNumberToObject(root, "network", 2);

  cJSON *data = cJSON_AddObjectToObject(root, "data");
  cJSON_AddStringToObject(data, "uid", "");
  cJSON_AddNumberToObject(data, "leds", 0);
  cJSON_AddNumberToObject(data, "space", 0);

  const char *info = cJSON_Print(root);
  httpd_resp_sendstr(req, info);
  free((void *)info);

  cJSON_Delete(root);
  return ESP_OK;
}

static esp_err_t api_post_handler(httpd_req_t *req)
{
  ESP_LOGI(TAG, "API POST request");

  set_cors_headers(req);
  httpd_resp_set_type(req, "application/json");

  return ESP_OK;
}

esp_err_t init_server(void)
{
  ESP_LOGI(TAG, "Initializing server");

  server_context_t *context = calloc(1, sizeof(server_context_t));
  GOTO_CHECK(context == NULL, TAG, "Failed to allocate memory for server context", error);

  snprintf(
      context->base_path,
      FILE_SYSTEM_BASE_PATH_MAX_LENGTH,
      "%s%s",
      FILE_SYSTEM_BASE_PATH, FILE_SYSTEM_PUBLIC_BASE_PATH);

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  GOTO_CHECK(httpd_start(&server, &config), TAG, "Failed to start server", error_start);

  httpd_uri_t api_get_uri = {
      .uri = "/api",
      .method = HTTP_GET,
      .handler = api_get_handler,
      .user_ctx = context};
  httpd_register_uri_handler(server, &api_get_uri);

  httpd_uri_t api_post_uri = {
      .uri = "/api",
      .method = HTTP_POST,
      .handler = api_post_handler,
      .user_ctx = context};
  httpd_register_uri_handler(server, &api_post_uri);

  httpd_uri_t common_get_uri = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = common_get_handler,
      .user_ctx = context};
  httpd_register_uri_handler(server, &common_get_uri);

  return ESP_OK;
error_start:
  free(context);
error:
  return ESP_FAIL;
}
