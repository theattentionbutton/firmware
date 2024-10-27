#include <FS.h>
#include <LittleFS.h>

#ifndef __LFS_KV
#define __LFS_KV

#define KV_SIZE_MAX 255
#define KEY(x, y)                                                              \
    char y[KV_SIZE_MAX] = {0};                                                 \
    get_key_filename(x, y)

void get_key_filename(const char *key, char *buf) {
    snprintf(buf, KV_SIZE_MAX - 1, "/%s", key);
}

size_t kv_put(const char *key, const char *val) {
    KEY(key, buf);
    auto f = LittleFS.open(buf, "w");
    if (!f || f.isDirectory()) return 0;
    int bytes = f.print(val);
    f.close();
    return bytes;
}

size_t kv_get(const char *key, char *dest, size_t len) {
    KEY(key, buf);
    if (!LittleFS.exists(buf)) return 0;
    auto f = LittleFS.open(buf, "r");
    if (!f || f.isDirectory()) return 0;
    memset(dest, 0, len);
    return f.readBytes(dest, len);
}

int kv_remove(const char *key) {
    KEY(key, buf);
    return LittleFS.remove(buf);
}
#endif
