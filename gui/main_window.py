import sys
import os
import ctypes
from PyQt5.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                             QTabWidget, QGroupBox, QLabel, QLineEdit, QPushButton,
                             QComboBox, QProgressBar, QTextEdit, QFileDialog,
                             QMessageBox, QListWidget, QSplitter, QAction, QToolBar)
from PyQt5.QtCore import Qt, QThread, pyqtSignal, QTimer
from PyQt5.QtGui import QIcon, QFont

from .auth_manager import AuthManager
from .device_manager import DeviceManager
from .flash_worker import FlashWorker

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("SPDR Client - Transsion Tool")
        self.setGeometry(100, 100, 1000, 700)
        
        # Initialize core library
        self.core_lib = None
        self.load_core_library()
        
        # Initialize managers
        self.auth_manager = AuthManager(self.core_lib)
        self.device_manager = DeviceManager(self.core_lib)
        
        # Setup UI
        self.setup_ui()
        
        # Refresh devices periodically
        self.device_refresh_timer = QTimer(self)
        self.device_refresh_timer.timeout.connect(self.refresh_devices)
        self.device_refresh_timer.start(5000)  # Refresh every 5 seconds
        
    def load_core_library(self):
        """Load the C++ core library"""
        try:
            if sys.platform == "win32":
                lib_path = os.path.join("core", "spdr_core.dll")
            elif sys.platform == "darwin":
                lib_path = os.path.join("core", "libspdr_core.dylib")
            else:
                lib_path = os.path.join("core", "libspdr_core.so")
                
            self.core_lib = ctypes.CDLL(lib_path)
            
            # Set up function prototypes
            self.setup_function_prototypes()
            
            # Initialize the library
            if not self.core_lib.spdr_init():
                error = self.core_lib.spdr_get_error()
                QMessageBox.critical(self, "Error", f"Failed to initialize core library: {error}")
                
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load core library: {str(e)}")
            
    def setup_function_prototypes(self):
        """Setup function prototypes for the core library"""
        # Device management
        self.core_lib.spdr_init.restype = ctypes.c_bool
        self.core_lib.spdr_cleanup.restype = ctypes.c_bool
        self.core_lib.spdr_scan_devices.restype = ctypes.c_int
        self.core_lib.spdr_scan_devices.argtypes = [
            ctypes.c_uint16, ctypes.c_uint16, ctypes.c_char_p, ctypes.c_int
        ]
        self.core_lib.spdr_connect.restype = ctypes.c_bool
        self.core_lib.spdr_connect.argtypes = [ctypes.c_int]
        self.core_lib.spdr_disconnect.restype = ctypes.c_bool
        
        # Authentication
        self.core_lib.spdr_auth_transsion.restype = ctypes.c_bool
        self.core_lib.spdr_auth_transsion.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.core_lib.spdr_auth_infinix.restype = ctypes.c_bool
        self.core_lib.spdr_auth_tecno.restype = ctypes.c_bool
        self.core_lib.spdr_auth_itel.restype = ctypes.c_bool
        
        # Utility functions
        self.core_lib.spdr_get_error.restype = ctypes.c_char_p
        self.core_lib.spdr_get_progress.restype = ctypes.c_int
        
    def setup_ui(self):
        """Setup the main UI"""
        # Create central widget and layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        
        # Create toolbar
        self.setup_toolbar()
        
        # Create tab widget
        tab_widget = QTabWidget()
        main_layout.addWidget(tab_widget)
        
        # Add tabs
        tab_widget.addTab(self.create_device_tab(), "Device")
        tab_widget.addTab(self.create_flash_tab(), "Flash")
        tab_widget.addTab(self.create_auth_tab(), "Authentication")
        tab_widget.addTab(self.create_log_tab(), "Log")
        
        # Status bar
        self.statusBar().showMessage("Ready")
        
    def setup_toolbar(self):
        """Setup the toolbar"""
        toolbar = QToolBar("Main Toolbar")
        self.addToolBar(toolbar)
        
        # Add actions
        refresh_action = QAction(QIcon("resources/icons/refresh.png"), "Refresh Devices", self)
        refresh_action.triggered.connect(self.refresh_devices)
        toolbar.addAction(refresh_action)
        
        connect_action = QAction(QIcon("resources/icons/connect.png"), "Connect", self)
        connect_action.triggered.connect(self.connect_device)
        toolbar.addAction(connect_action)
        
        disconnect_action = QAction(QIcon("resources/icons/disconnect.png"), "Disconnect", self)
        disconnect_action.triggered.connect(self.disconnect_device)
        toolbar.addAction(disconnect_action)
        
        toolbar.addSeparator()
        
        auth_action = QAction(QIcon("resources/icons/auth.png"), "Authenticate", self)
        auth_action.triggered.connect(self.show_auth_dialog)
        toolbar.addAction(auth_action)
        
    def create_device_tab(self):
        """Create the device tab"""
        device_tab = QWidget()
        layout = QVBoxLayout(device_tab)
        
        # Device selection group
        device_group = QGroupBox("Device Selection")
        device_layout = QVBoxLayout(device_group)
        
        # Device list
        self.device_list = QListWidget()
        device_layout.addWidget(QLabel("Available Devices:"))
        device_layout.addWidget(self.device_list)
        
        # Refresh button
        refresh_btn = QPushButton("Refresh Devices")
        refresh_btn.clicked.connect(self.refresh_devices)
        device_layout.addWidget(refresh_btn)
        
        # Device info group
        info_group = QGroupBox("Device Information")
        info_layout = QVBoxLayout(info_group)
        
        self.device_info = QTextEdit()
        self.device_info.setReadOnly(True)
        info_layout.addWidget(self.device_info)
        
        # Add groups to layout
        layout.addWidget(device_group)
        layout.addWidget(info_group)
        
        return device_tab
        
    def create_flash_tab(self):
        """Create the flash tab"""
        flash_tab = QWidget()
        layout = QVBoxLayout(flash_tab)
        
        # File selection group
        file_group = QGroupBox("File Selection")
        file_layout = QVBoxLayout(file_group)
        
        # FDL1 selection
        fdl1_layout = QHBoxLayout()
        fdl1_layout.addWidget(QLabel("FDL1:"))
        self.fdl1_path = QLineEdit()
        fdl1_layout.addWidget(self.fdl1_path)
        fdl1_browse_btn = QPushButton("Browse")
        fdl1_browse_btn.clicked.connect(lambda: self.browse_file(self.fdl1_path))
        fdl1_layout.addWidget(fdl1_browse_btn)
        file_layout.addLayout(fdl1_layout)
        
        # FDL2 selection
        fdl2_layout = QHBoxLayout()
        fdl2_layout.addWidget(QLabel("FDL2:"))
        self.fdl2_path = QLineEdit()
        fdl2_layout.addWidget(self.fdl2_path)
        fdl2_browse_btn = QPushButton("Browse")
        fdl2_browse_btn.clicked.connect(lambda: self.browse_file(self.fdl2_path))
        fdl2_layout.addWidget(fdl2_browse_btn)
        file_layout.addLayout(fdl2_layout)
        
        # Firmware selection
        fw_layout = QHBoxLayout()
        fw_layout.addWidget(QLabel("Firmware:"))
        self.fw_path = QLineEdit()
        fw_layout.addWidget(self.fw_path)
        fw_browse_btn = QPushButton("Browse")
        fw_browse_btn.clicked.connect(lambda: self.browse_file(self.fw_path))
        fw_layout.addWidget(fw_browse_btn)
        file_layout.addLayout(fw_layout)
        
        # Flash options group
        options_group = QGroupBox("Flash Options")
        options_layout = QVBoxLayout(options_group)
        
        # Flash mode selection
        mode_layout = QHBoxLayout()
        mode_layout.addWidget(QLabel("Flash Mode:"))
        self.flash_mode = QComboBox()
        self.flash_mode.addItems(["Normal", "Force", "Verification Only"])
        mode_layout.addWidget(self.flash_mode)
        options_layout.addLayout(mode_layout)
        
        # Progress bar
        self.progress_bar = QProgressBar()
        options_layout.addWidget(self.progress_bar)
        
        # Flash button
        self.flash_btn = QPushButton("Start Flashing")
        self.flash_btn.clicked.connect(self.start_flashing)
        options_layout.addWidget(self.flash_btn)
        
        # Add groups to layout
        layout.addWidget(file_group)
        layout.addWidget(options_group)
        
        return flash_tab
        
    def create_auth_tab(self):
        """Create the authentication tab"""
        auth_tab = QWidget()
        layout = QVBoxLayout(auth_tab)
        
        # Brand selection
        brand_group = QGroupBox("Device Brand")
        brand_layout = QVBoxLayout(brand_group)
        
        self.brand_combo = QComboBox()
        self.brand_combo.addItems(["Infinix", "Tecno", "Itel"])
        brand_layout.addWidget(self.brand_combo)
        
        # Auth status
        status_group = QGroupBox("Authentication Status")
        status_layout = QVBoxLayout(status_group)
        
        self.auth_status = QLabel("Not Authenticated")
        self.auth_status.setAlignment(Qt.AlignCenter)
        self.auth_status.setStyleSheet("QLabel { background-color: #ffcccc; padding: 10px; }")
        status_layout.addWidget(self.auth_status)
        
        # Auth button
        self.auth_btn = QPushButton("Authenticate Device")
        self.auth_btn.clicked.connect(self.authenticate_device)
        
        # Add groups to layout
        layout.addWidget(brand_group)
        layout.addWidget(status_group)
        layout.addWidget(self.auth_btn)
        
        return auth_tab
        
    def create_log_tab(self):
        """Create the log tab"""
        log_tab = QWidget()
        layout = QVBoxLayout(log_tab)
        
        self.log_text = QTextEdit()
        self.log_text.setReadOnly(True)
        layout.addWidget(self.log_text)
        
        # Clear log button
        clear_btn = QPushButton("Clear Log")
        clear_btn.clicked.connect(self.clear_log)
        layout.addWidget(clear_btn)
        
        return log_tab
        
    def refresh_devices(self):
        """Refresh the list of available devices"""
        if not self.core_lib:
            self.log_message("Core library not loaded")
            return
            
        # Scan for devices
        buffer_size = 1024
        buffer = ctypes.create_string_buffer(buffer_size)
        result = self.core_lib.spdr_scan_devices(0x1782, 0x4D00, buffer, buffer_size)
        
        if result > 0:
            devices = buffer.value.decode('utf-8').split('\n')
            self.device_list.clear()
            self.device_list.addItems([d for d in devices if d])
        else:
            error = self.core_lib.spdr_get_error()
            self.log_message(f"Error scanning devices: {error}")
            
    def connect_device(self):
        """Connect to the selected device"""
        if not self.device_list.currentItem():
            QMessageBox.warning(self, "Warning", "Please select a device first")
            return
            
        device_index = self.device_list.currentRow()
        if self.core_lib.spdr_connect(device_index):
            self.log_message("Device connected successfully")
            self.statusBar().showMessage("Device connected")
        else:
            error = self.core_lib.spdr_get_error()
            self.log_message(f"Failed to connect: {error}")
            QMessageBox.critical(self, "Error", f"Failed to connect: {error}")
            
    def disconnect_device(self):
        """Disconnect from the current device"""
        if self.core_lib.spdr_disconnect():
            self.log_message("Device disconnected")
            self.statusBar().showMessage("Device disconnected")
        else:
            error = self.core_lib.spdr_get_error()
            self.log_message(f"Failed to disconnect: {error}")
            
    def browse_file(self, line_edit):
        """Open a file dialog and set the path to the line edit"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Select File", "", "Binary Files (*.bin *.mbn);;All Files (*)"
        )
        if file_path:
            line_edit.setText(file_path)
            
    def authenticate_device(self):
        """Authenticate the device based on selected brand"""
        brand = self.brand_combo.currentText().lower()
        
        if brand == "infinix":
            success = self.core_lib.spdr_auth_infinix()
        elif brand == "tecno":
            success = self.core_lib.spdr_auth_tecno()
        elif brand == "itel":
            success = self.core_lib.spdr_auth_itel()
        else:
            success = False
            
        if success:
            self.auth_status.setText("Authentication Successful")
            self.auth_status.setStyleSheet("QLabel { background-color: #ccffcc; padding: 10px; }")
            self.log_message(f"{brand.capitalize()} authentication successful")
        else:
            error = self.core_lib.spdr_get_error()
            self.auth_status.setText("Authentication Failed")
            self.auth_status.setStyleSheet("QLabel { background-color: #ffcccc; padding: 10px; }")
            self.log_message(f"Authentication failed: {error}")
            
    def start_flashing(self):
        """Start the flashing process"""
        # Validate inputs
        if not self.fdl1_path.text() or not self.fdl2_path.text() or not self.fw_path.text():
            QMessageBox.warning(self, "Warning", "Please select all required files")
            return
            
        # Create and start flash worker
        self.flash_worker = FlashWorker(
            self.core_lib,
            self.fdl1_path.text(),
            self.fdl2_path.text(),
            self.fw_path.text(),
            self.flash_mode.currentIndex()
        )
        
        # Connect signals
        self.flash_worker.progress_updated.connect(self.update_progress)
        self.flash_worker.message_logged.connect(self.log_message)
        self.flash_worker.finished.connect(self.flash_finished)
        
        # Disable UI during flash
        self.set_ui_enabled(False)
        
        # Start the worker
        self.flash_worker.start()
        
    def update_progress(self, value):
        """Update the progress bar"""
        self.progress_bar.setValue(value)
        
    def flash_finished(self, success):
        """Handle flash completion"""
        self.set_ui_enabled(True)
        if success:
            self.log_message("Flashing completed successfully")
            QMessageBox.information(self, "Success", "Flashing completed successfully")
        else:
            self.log_message("Flashing failed")
            QMessageBox.critical(self, "Error", "Flashing failed")
            
    def set_ui_enabled(self, enabled):
        """Enable or disable UI elements during flash"""
        self.flash_btn.setEnabled(enabled)
        self.fdl1_path.setEnabled(enabled)
        self.fdl2_path.setEnabled(enabled)
        self.fw_path.setEnabled(enabled)
        self.flash_mode.setEnabled(enabled)
        
    def log_message(self, message):
        """Add a message to the log"""
        self.log_text.append(f"{message}")
        
    def clear_log(self):
        """Clear the log"""
        self.log_text.clear()
        
    def closeEvent(self, event):
        """Handle application close"""
        if self.core_lib:
            self.core_lib.spdr_cleanup()
        event.accept()