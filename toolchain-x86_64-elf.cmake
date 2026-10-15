set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Force CMake to skip running ABI checks with host linkers on bare-metal targets
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Path to your cross-compiler toolchain binaries
set(TOOLCHAIN_PREFIX "/home/charlotte/Projects/celesteos/x86_64-elf-cross/bin/x86_64-elf-")

# Compiler cross-executables
set(CMAKE_C_COMPILER   "${TOOLCHAIN_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++")
set(CMAKE_AR           "${TOOLCHAIN_PREFIX}ar")
set(CMAKE_RANLIB       "${TOOLCHAIN_PREFIX}ranlib")

# Set search roots to avoid picking up host system libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)