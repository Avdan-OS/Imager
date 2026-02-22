cmake_minimum_required(VERSION 3.10)
project(Imager VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/core/progress.c
    src/utils/utils.c
    src/core/iso_operations.c
)

# Create executable
add_executable(imager ${SOURCES})

# Set compiler flags
if(MSVC)
    target_compile_options(imager PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
    # /O2 is incompatible with /RTC1 (default in Debug). CMake handles /O2 in Release by default.
else()
    target_compile_options(imager PRIVATE -O2 -Wall)
endif()

# Install target
install(TARGETS imager DESTINATION bin)