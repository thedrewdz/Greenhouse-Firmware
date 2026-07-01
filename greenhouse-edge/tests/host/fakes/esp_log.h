#ifndef GH_HOST_FAKE_ESP_LOG_H
#define GH_HOST_FAKE_ESP_LOG_H

/* No-op logging for host tests. Arguments are evaluated for -Wformat safety. */
#include <stdio.h>

#define GH_FAKE_LOG(fmt, ...) do { if (0) { (void)printf(fmt, ##__VA_ARGS__); } } while (0)

#define ESP_LOGE(tag, fmt, ...) GH_FAKE_LOG(fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) GH_FAKE_LOG(fmt, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) GH_FAKE_LOG(fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) GH_FAKE_LOG(fmt, ##__VA_ARGS__)
#define ESP_LOGV(tag, fmt, ...) GH_FAKE_LOG(fmt, ##__VA_ARGS__)

#endif
