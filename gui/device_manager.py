import ctypes

class DeviceManager:
    def __init__(self, core_lib):
        self.core_lib = core_lib
        self.connected = False
        self.device_info = {}
        
    def scan_devices(self):
        """Scan for available devices"""
        buffer_size = 1024
        buffer = ctypes.create_string_buffer(buffer_size)
        result = self.core_lib.spdr_scan_devices(0x1782, 0x4D00, buffer, buffer_size)
        
        if result > 0:
            devices = buffer.value.decode('utf-8').split('\n')
            return [d for d in devices if d]
        else:
            error = self.core_lib.spdr_get_error()
            raise Exception(f"Scan failed: {error}")
            
    def connect(self, device_index):
        """Connect to a device"""
        if self.core_lib.spdr_connect(device_index):
            self.connected = True
            return True
        else:
            error = self.core_lib.spdr_get_error()
            raise Exception(f"Connection failed: {error}")
            
    def disconnect(self):
        """Disconnect from the current device"""
        if self.core_lib.spdr_disconnect():
            self.connected = False
            return True
        else:
            error = self.core_lib.spdr_get_error()
            raise Exception(f"Disconnection failed: {error}")
            
    def is_connected(self):
        """Check if device is connected"""
        return self.connected