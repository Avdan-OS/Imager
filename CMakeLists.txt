cmake_minimum_required(VERSION 3.10)
project(Imager VERSION 1.0.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Optional streaming decompression codecs
set(DECOMP_LIBS "")
set(DECOMP_DEFS "")
set(DECOMP_INCLUDES "")
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    list(APPEND DECOMP_LIBS ZLIB::ZLIB)
    list(APPEND DECOMP_DEFS HAVE_ZLIB)
endif()
find_package(LibLZMA QUIET)
if(LIBLZMA_FOUND)
    list(APPEND DECOMP_LIBS LibLZMA::LibLZMA)
    list(APPEND DECOMP_DEFS HAVE_LZMA)
endif()
find_package(BZip2 QUIET)
if(BZIP2_FOUND)
    list(APPEND DECOMP_LIBS BZip2::BZip2)
    list(APPEND DECOMP_DEFS HAVE_BZIP2)
endif()
find_library(ZSTD_LIBRARY zstd)
find_path(ZSTD_INCLUDE_DIR zstd.h)
if(ZSTD_LIBRARY AND ZSTD_INCLUDE_DIR)
    list(APPEND DECOMP_LIBS ${ZSTD_LIBRARY})
    list(APPEND DECOMP_DEFS HAVE_ZSTD)
    list(APPEND DECOMP_INCLUDES ${ZSTD_INCLUDE_DIR})
endif()
if(DECOMP_DEFS)
    message(STATUS "Imager: streaming decompression enabled: ${DECOMP_DEFS}")
else()
    message(STATUS "Imager: no decompression codecs found; compressed images will be rejected")
endif()

# Source files
set(SOURCES
    src/main.c
    src/core/progress.c
    src/utils/utils.c
    src/core/iso_operations.c
    src/core/image_format.c
    src/core/image_reader.c
    src/core/image_write.c
    src/core/windows_iso.c
    src/core/device_lock.c
)

# Create executable
add_executable(imager ${SOURCES})
target_compile_definitions(imager PRIVATE ${DECOMP_DEFS})
target_link_libraries(imager PRIVATE ${DECOMP_LIBS})
if(DECOMP_INCLUDES)
    target_include_directories(imager PRIVATE ${DECOMP_INCLUDES})
endif()

# Set compiler flags
if(MSVC)
    target_compile_options(imager PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
    # /O2 is incompatible with /RTC1 (default in Debug). CMake handles /O2 in Release by default.
else()
    target_compile_options(imager PRIVATE -O2 -Wall)
endif()

# Fetch wxWidgets for Native GUI
include(FetchContent)
FetchContent_Declare(
  wxWidgets
  URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.8.1/wxWidgets-3.2.8.1.tar.bz2
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# wxWidgets build can be slow, we only need core and base
set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(wxWidgets)

# GUI Source files (Transitioned to C++)
set(GUI_SOURCES
    src/gui_main.cpp
    src/core/progress.c
    src/utils/utils.c
    src/utils/drive_list.c
    src/core/iso_operations.c
    src/core/image_format.c
    src/core/image_reader.c
    src/core/image_write.c
    src/core/windows_iso.c
    src/core/device_lock.c
)

# Create GUI executable
add_executable(imager-gui ${GUI_SOURCES})
target_include_directories(imager-gui PRIVATE include)
target_compile_definitions(imager-gui PRIVATE ${DECOMP_DEFS})
target_link_libraries(imager-gui PRIVATE ${DECOMP_LIBS})
if(DECOMP_INCLUDES)
    target_include_directories(imager-gui PRIVATE ${DECOMP_INCLUDES})
endif()

# Link wxWidgets
target_link_libraries(imager-gui PRIVATE wx::core wx::base)

if(MSVC)
    target_compile_options(imager-gui PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
    # Win32 subsystem for GUI (no console window)
    set_target_properties(imager-gui PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    target_compile_options(imager-gui PRIVATE -O2 -Wall)
    target_link_libraries(imager-gui PRIVATE pthread m dl)
endif()

# Install target
install(TARGETS imager imager-gui DESTINATION bin)
