#include "SPIFFS.h"

#ifndef __SPI_FFS_KV
#define __SPI_FFS_KV

#define KV_SIZE_MAX 255
#define KEY(x, y) char y[KV_SIZE_MAX] = {0}; get_key_filename(x, y)

void get_key_filename(const char *key, char *buf) {
  snprintf(buf, KV_SIZE_MAX - 1, "/%s", key);
}

size_t kv_put(const char *key, const char *val) {
  KEY(key, buf);
  auto f = SPIFFS.open(buf, FILE_WRITE);
  if (!f || f.isDirectory()) return 0;  
  int bytes = f.print(val);
  f.close();
  return bytes;
}

size_t kv_get(const char *key, char *dest, size_t len) {
  KEY(key, buf);
  if (!SPIFFS.exists(buf)) return 0;
  auto f = SPIFFS.open(buf, FILE_READ);
  if (!f || f.isDirectory()) return 0;
  memset(dest, 0, len);
  return f.readBytes(dest, len);
}

int kv_remove(const char *key) {
  KEY(key, buf);
  return SPIFFS.remove(buf);
}
#endif
