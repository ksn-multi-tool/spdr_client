# build.py
import os
import platform
import subprocess
import sys

def build_core():
    """Build the C++ core library"""
    if platform.system() == "Windows":
        compiler = "g++"
        lib_name = "spdr_core.dll"
        compile_cmd = [
            compiler, "-shared", "-fPIC",
            "-I", "core",
            "-DBUILD_LIB",
            "-Wno-unused-parameter",
            "core/BMPlatform.cpp", "core/common.cpp", "core/spd_dump.c", "core/Wrapper.cpp",
            "-o", f"core/{lib_name}",
            "-lsetupapi"  # Добавляем библиотеку setupapi
        ]
        if USE_LIBUSB:
            compile_cmd.insert(-2, "-llibusb-1.0")
    elif platform.system() == "Darwin":
        compiler = "clang++"
        lib_name = "libspdr_core.dylib"
        compile_cmd = [
            compiler, "-dynamiclib", "-fPIC",
            "-I", "core",
            "-DBUILD_LIB",
            "-Wno-unused-parameter",
            "core/BMPlatform.cpp", "core/common.cpp", "core/spd_dump.c", "core/Wrapper.cpp",
            "-o", f"core/{lib_name}"
        ]
        if USE_LIBUSB:
            compile_cmd.insert(-2, "-lusb-1.0")
    else:
        compiler = "g++"
        lib_name = "libspdr_core.so"
        compile_cmd = [
            compiler, "-shared", "-fPIC",
            "-I", "core",
            "-DBUILD_LIB",
            "-Wno-unused-parameter",
            "core/BMPlatform.cpp", "core/common.cpp", "core/spd_dump.c", "core/Wrapper.cpp",
            "-o", f"core/{lib_name}"
        ]
        if USE_LIBUSB:
            compile_cmd.insert(-2, "-lusb-1.0")
    
    compile_cmd = [arg for arg in compile_cmd if arg != ""]
    
    compiler_found = False
    compiler_name = compiler
    if platform.system() == "Windows":
        compiler_name += ".exe"
    for path in os.environ["PATH"].split(os.pathsep):
        compiler_path = os.path.join(path, compiler_name)
        if os.path.exists(compiler_path):
            compiler_found = True
            break
    
    if not compiler_found:
        print(f"Error: {compiler} not found in PATH")
        return False
    
    os.makedirs("core", exist_ok=True)
    
    try:
        print("Building core library...")
        print("Command:", " ".join(compile_cmd))
        
        result = subprocess.run(compile_cmd, capture_output=True, text=True, cwd=os.getcwd())
        if result.returncode != 0:
            print("Build failed:")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            return False
        
        print("Core library built successfully")
        return True
        
    except Exception as e:
        print(f"Build error: {str(e)}")
        return False

def check_dependencies():
    """Check if required source files exist"""
    required_files = [
        "core/BMPlatform.cpp",
        "core/BMPlatform.h", 
        "core/common.cpp",
        "core/common.h",
        "core/spd_dump.c", 
        "core/spd_cmd.h", 
        "core/Wrapper.cpp",
        "core/Wrapper.h",
        "core/GITVER.h"
    ]
    
    missing_files = []
    for file in required_files:
        if not os.path.exists(file):
            missing_files.append(file)
    
    if missing_files:
        print("Missing required files:")
        for file in missing_files:
            print(f"  - {file}")
        return False
    
    return True

USE_LIBUSB = platform.system() != "Windows"

if __name__ == "__main__":
    print("SPDR Client Build Script")
    print("Platform:", platform.system())
    print("Using libusb:", USE_LIBUSB)
    
    if not check_dependencies():
        print("Please make sure all required source files are in the core directory")
        sys.exit(1)
    
    if build_core():
        print("Build completed successfully")
        sys.exit(0)
    else:
        print("Build failed")
        sys.exit(1)