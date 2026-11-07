#pragma once
// Test double for the ESP-IDF NVS C API. The production journal .cpp is
// compiled unchanged against these declarations; this is NOT a device test.
#include <cstddef>
#include <cstdint>

using esp_err_t = int;
using nvs_handle_t = std::uint32_t;
inline constexpr esp_err_t ESP_OK = 0;
inline constexpr esp_err_t ESP_FAIL = -1;
inline constexpr esp_err_t ESP_ERR_NVS_NOT_FOUND = 0x1102;
inline constexpr esp_err_t ESP_ERR_NVS_INVALID_LENGTH = 0x110c;
inline constexpr int NVS_READONLY = 0;
inline constexpr int NVS_READWRITE = 1;

esp_err_t nvs_open(const char* namespace_name, int mode, nvs_handle_t* handle);
void nvs_close(nvs_handle_t handle);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key,
                       void* value, std::size_t* length);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key,
                       const void* value, std::size_t length);
esp_err_t nvs_get_u8(nvs_handle_t handle, const char* key, std::uint8_t* value);
esp_err_t nvs_set_u8(nvs_handle_t handle, const char* key, std::uint8_t value);
esp_err_t nvs_erase_all(nvs_handle_t handle);
esp_err_t nvs_commit(nvs_handle_t handle);
