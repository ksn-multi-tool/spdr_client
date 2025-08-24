import sys
import os
from PyQt5.QtWidgets import QApplication
from gui.main_window import MainWindow

if __name__ == "__main__":
    # Add the core directory to the DLL search path
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(os.path.join(os.path.dirname(__file__), "core"))
    
    app = QApplication(sys.argv)
    
    # Set application style
    app.setStyle("Fusion")
    
    # Create and show main window
    window = MainWindow()
    window.show()
    
    # Run the application
    sys.exit(app.exec_())