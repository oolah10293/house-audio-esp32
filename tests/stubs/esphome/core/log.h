#pragma once
#include <cstdio>
#define ESP_LOGI(tag,fmt,...) do { (void)(tag); std::printf(fmt "\n", ##__VA_ARGS__); } while (0)
#define ESP_LOGE ESP_LOGI
#define ESP_LOGCONFIG ESP_LOGI
