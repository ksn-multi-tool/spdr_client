#pragma once

#ifdef _WIN32
#ifdef CORE_EXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
#else
#define CORE_API
#endif

#include <cstdint>

extern "C" {
    // Device management
    CORE_API bool spdr_init();
    CORE_API bool spdr_cleanup();
    CORE_API int spdr_scan_devices(uint16_t vid, uint16_t pid, char* result, int max_len);
    CORE_API bool spdr_connect(int device_index);
    CORE_API bool spdr_disconnect();
    
    // Authentication
    CORE_API bool spdr_auth_transsion(const char* key_name, const char* key_value);
    CORE_API bool spdr_auth_infinix();
    CORE_API bool spdr_auth_tecno();
    CORE_API bool spdr_auth_itel();
    
    // Flash operations
    CORE_API bool spdr_load_fdl(const char* fdl_path, int fdl_type);
    CORE_API bool spdr_flash_start();
    CORE_API bool spdr_flash_write(const uint8_t* data, size_t length, size_t offset);
    CORE_API bool spdr_flash_finish();
    CORE_API bool spdr_read_memory(size_t offset, uint8_t* buffer, size_t length);
    
    // Utility functions
    CORE_API const char* spdr_get_error();
    CORE_API int spdr_get_progress();
}