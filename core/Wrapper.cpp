#include "Wrapper.h"
#include "BMPlatform.h"
#include "common.h"
#include <vector>
#include <string>

// Global variables
static CBootModeOpr* g_bmOpr = nullptr;
static std::string g_last_error;
static int g_progress = 0;

// Initialize the library
CORE_API bool spdr_init() {
    try {
        g_bmOpr = new CBootModeOpr();
        if (!g_bmOpr->Initialize()) {
            g_last_error = "Failed to initialize BMPlatform";
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        g_last_error = e.what();
        return false;
    }
}

// Cleanup resources
CORE_API bool spdr_cleanup() {
    try {
        if (g_bmOpr) {
            g_bmOpr->Uninitialize();
            delete g_bmOpr;
            g_bmOpr = nullptr;
        }
        return true;
    } catch (const std::exception& e) {
        g_last_error = e.what();
        return false;
    }
}

// Scan for devices
CORE_API int spdr_scan_devices(uint16_t vid, uint16_t pid, char* result, int max_len) {
    // Implementation using your common.c functions
#if !USE_LIBUSB
    DWORD* ports = FindPort("SPRD U2S Diag");
    if (!ports) {
        g_last_error = "No devices found";
        return 0;
    }
    
    std::string device_list;
    for (DWORD* port = ports; *port != 0; port++) {
        device_list += "COM" + std::to_string(*port) + "\n";
    }
    free(ports);
    
    if (device_list.length() > max_len - 1) {
        g_last_error = "Buffer too small";
        return 0;
    }
    
    strncpy(result, device_list.c_str(), max_len);
    return device_list.length();
#else
    // LIBUSB implementation
    libusb_device** devices = FindPort(pid);
    if (!devices) {
        g_last_error = "No devices found";
        return 0;
    }
    
    std::string device_list;
    int count = 0;
    for (libusb_device** dev = devices; *dev != nullptr; dev++) {
        count++;
        device_list += "USB Device " + std::to_string(count) + "\n";
        libusb_unref_device(*dev);
    }
    free(devices);
    
    if (device_list.length() > max_len - 1) {
        g_last_error = "Buffer too small";
        return 0;
    }
    
    strncpy(result, device_list.c_str(), max_len);
    return device_list.length();
#endif
}

// Connect to device
CORE_API bool spdr_connect(int device_index) {
    if (!g_bmOpr) {
        g_last_error = "Library not initialized";
        return false;
    }
    
    // Implementation would connect to the specific device
    // This is a simplified version
#if !USE_LIBUSB
    DWORD* ports = FindPort("SPRD U2S Diag");
    if (!ports || device_index < 0) {
        g_last_error = "Invalid device index";
        return false;
    }
    
    DWORD port = ports[device_index];
    if (port == 0) {
        g_last_error = "Invalid device index";
        free(ports);
        return false;
    }
    
    bool result = g_bmOpr->ConnectChannel(port, 0, 0);
    free(ports);
    return result;
#else
    // LIBUSB implementation
    g_last_error = "USB connection not implemented in this wrapper";
    return false;
#endif
}

// Transsion authentication functions
CORE_API bool spdr_auth_transsion(const char* key_name, const char* key_value) {
    // Implementation would use the provided keys to authenticate
    // This is a placeholder for the actual authentication logic
    g_last_error = "Transsion authentication not implemented";
    return false;
}

CORE_API bool spdr_auth_infinix() {
    return spdr_auth_transsion("infinix_sprd_key", "11223344556677889900AABBCCDDEEFF");
}

CORE_API bool spdr_auth_tecno() {
    return spdr_auth_transsion("tecno_sprd_key", "3344556677889900AABBCCDDEEFF0011");
}

CORE_API bool spdr_auth_itel() {
    return spdr_auth_transsion("itel_sprd_key", "556677889900AABBCCDDEEFF11223344");
}

// Error handling
CORE_API const char* spdr_get_error() {
    return g_last_error.c_str();
}

CORE_API int spdr_get_progress() {
    return g_progress;
}