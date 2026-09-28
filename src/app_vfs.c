#include "app_vfs.h"
#include <dirent.h>

static const char *TAG = "APP_FILE_SYSTEM";

static vfs_size_t _cached_vfs_size = {0};
static bool _vfs_size_valid = false;

esp_vfs_littlefs_conf_t conf = {
  .base_path = FILE_SYSTEM_BASE_PATH,
  .partition_label = CONFIG_APP_FILE_SYSTEM_PARTITION_LABEL,
  .format_if_mount_failed = true,
  .dont_mount = false,
};

esp_err_t init_vfs(void)
{
  ESP_LOGI(TAG, "Initializing file system");

  GOTO_CHECK(esp_vfs_littlefs_register(&conf) != ESP_OK, TAG, "Failed to register LittleFS", error);

  _cached_vfs_size = get_vfs_space_info();

  return ESP_OK;
error:
  return ESP_FAIL;
}

char *clean_vfs_path(char *path)
{
  if (path == NULL)
  {
    return NULL;
  }

  size_t path_length = strlen(path);
  char *new_path = path;

  for (int i = 0; i < path_length; i++)
  {
    if (strchr(ALLOWED_PATH_CHARS, path[i]) == NULL)
    {
      new_path[i] = FORBIDDEN_CHARACTERS_PLACEHOLDER;
    }
  }

  return new_path;
}

vfs_size_t get_vfs_space_info(void)
{
  if (_vfs_size_valid)
  {
    return _cached_vfs_size;
  }

  vfs_size_t size = {0};

  esp_err_t err = esp_littlefs_info(conf.partition_label, &size.total, &size.used);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to get LittleFS info");
    return size;
  }

  size.free = size.total - size.used;

  _cached_vfs_size = size;
  _vfs_size_valid = true;

  return size;
}

void vfs_invalidate_space_cache(void)
{
  _vfs_size_valid = false;
}

esp_err_t vfs_make_dir(const char *path)
{
  ESP_LOGI(TAG, "Creating directory: %s", path);

  struct stat dir_stat;
  if (stat(path, &dir_stat) == 0)
  {
    ESP_LOGI(TAG, "Directory already exists: %s", path);
    return ESP_OK;
  }

  GOTO_CHECK(mkdir(path, 0755) != 0, TAG, "Failed to create directory", error);

  return ESP_OK;
error:
  return ESP_FAIL;
}

esp_err_t vfs_clean_dir(const char *path)
{
  ESP_LOGI(TAG, "Cleaning directory: %s", path);

  DIR *dir = opendir(path);
  if (dir == NULL)
  {
    return ESP_OK;
  }

  struct dirent *entry;
  char filepath[FILE_SYSTEM_PATH_MAX_LENGTH];

  while ((entry = readdir(dir)) != NULL)
  {
    snprintf(filepath, sizeof(filepath), "%.127s/%.127s", path, entry->d_name);
    unlink(filepath);
  }

  closedir(dir);
  return ESP_OK;
}

esp_err_t vfs_append_file(const char *path, const void *data, size_t size)
{
  ESP_LOGI(TAG, "Appending file: %s", path);

  FILE *file = fopen(path, "a");
  GOTO_CHECK(file == NULL, TAG, "Failed to open file", error);

  size_t written = fwrite(data, 1, size, file);
  GOTO_CHECK(written != size, TAG, "Failed to write file", error);

  fclose(file);

  return ESP_OK;
error:
  if (file != NULL)
  {
    fclose(file);
  }

  return ESP_FAIL;
}
