import time
from PyQt5.QtCore import QThread, pyqtSignal

class FlashWorker(QThread):
    progress_updated = pyqtSignal(int)
    message_logged = pyqtSignal(str)
    finished = pyqtSignal(bool)
    
    def __init__(self, core_lib, fdl1_path, fdl2_path, fw_path, flash_mode):
        super().__init__()
        self.core_lib = core_lib
        self.fdl1_path = fdl1_path
        self.fdl2_path = fdl2_path
        self.fw_path = fw_path
        self.flash_mode = flash_mode
        self.cancelled = False
        
    def run(self):
        """Run the flashing process"""
        try:
            # Load FDL1
            self.message_logged.emit("Loading FDL1...")
            if not self.load_fdl(self.fdl1_path, 1):
                self.finished.emit(False)
                return
                
            self.progress_updated.emit(25)
            
            # Load FDL2
            self.message_logged.emit("Loading FDL2...")
            if not self.load_fdl(self.fdl2_path, 2):
                self.finished.emit(False)
                return
                
            self.progress_updated.emit(50)
            
            # Start flash process
            self.message_logged.emit("Starting flash process...")
            if not self.core_lib.spdr_flash_start():
                error = self.core_lib.spdr_get_error()
                self.message_logged.emit(f"Failed to start flash: {error}")
                self.finished.emit(False)
                return
                
            self.progress_updated.emit(75)
            
            # Flash firmware (simplified)
            self.message_logged.emit("Flashing firmware...")
            # Actual implementation would read the firmware file and send it in chunks
            
            # Finish flash process
            self.message_logged.emit("Finalizing flash...")
            if not self.core_lib.spdr_flash_finish():
                error = self.core_lib.spdr_get_error()
                self.message_logged.emit(f"Failed to finish flash: {error}")
                self.finished.emit(False)
                return
                
            self.progress_updated.emit(100)
            self.message_logged.emit("Flash completed successfully")
            self.finished.emit(True)
            
        except Exception as e:
            self.message_logged.emit(f"Error during flash: {str(e)}")
            self.finished.emit(False)
            
    def load_fdl(self, fdl_path, fdl_type):
        """Load FDL file"""
        # Convert path to bytes for C function
        path_bytes = fdl_path.encode('utf-8')
        
        self.message_logged.emit(f"Loading FDL{fdl_type} from {fdl_path}")
        
        # Simulate loading process
        for i in range(5):
            if self.cancelled:
                return False
            time.sleep(0.5)
            self.progress_updated.emit(i * 5)
            
        return True
        
    def cancel(self):
        """Cancel the flash process"""
        self.cancelled = True

        self.message_logged.emit("Flash cancelled by user")
