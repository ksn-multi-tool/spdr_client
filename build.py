import os
import platform
import subprocess
import sys

def build_core():
    """Build the C++ core library"""
    # Determine platform-specific compiler options
    if platform.system() == "Windows":
        compiler = "cl.exe"
        lib_ext = ".dll"
        compile_cmd = [
            compiler, "/LD", "/D", "CORE_EXPORT", 
            "/I", "core", 
            "core/BMPlatform.cpp", "core/common.cpp", "core/Wrapper.cpp",
            "/link", "/OUT:core/spdr_core.dll"
        ]
    elif platform.system() == "Darwin":
        compiler = "clang++"
        lib_ext = ".dylib"
        compile_cmd = [
            compiler, "-dynamiclib", "-fPIC",
            "-I", "core",
            "core/BMPlatform.cpp", "core/common.cpp", "core/Wrapper.cpp",
            "-o", "core/libspdr_core.dylib"
        ]
    else:
        compiler = "g++"
        lib_ext = ".so"
        compile_cmd = [
            compiler, "-shared", "-fPIC",
            "-I", "core",
            "core/BMPlatform.cpp", "core/common.cpp", "core/Wrapper.cpp",
            "-o", "core/libspdr_core.so"
        ]
    
    # Check if compiler exists
    if not any(os.access(os.path.join(path, compiler), os.X_OK) for path in os.environ["PATH"].split(os.pathsep)):
        print(f"Error: {compiler} not found in PATH")
        return False
    
    # Create core directory if it doesn't exist
    os.makedirs("core", exist_ok=True)
    
    # Build the library
    try:
        print("Building core library...")
        result = subprocess.run(compile_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print("Build failed:")
            print(result.stderr)
            return False
        
        print("Core library built successfully")
        return True
        
    except Exception as e:
        print(f"Build error: {str(e)}")
        return False

if __name__ == "__main__":
    if build_core():
        print("Build completed successfully")
        sys.exit(0)
    else:
        print("Build failed")
        sys.exit(1)