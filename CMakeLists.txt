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

# Fetch wxWidgets for Native GUI
include(FetchContent)
FetchContent_Declare(
  wxWidgets
  URL https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.4/wxWidgets-3.2.4.tar.bz2
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
)

# Create GUI executable
add_executable(imager-gui ${GUI_SOURCES})
target_include_directories(imager-gui PRIVATE include)

# Link wxWidgets
target_link_libraries(imager-gui PRIVATE wx::core wx::base)

if(MSVC)
    target_compile_options(imager-gui PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
    # Win32 subsystem for GUI (no console window)
    set_target_properties(imager-gui PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    target_compile_options(imager-gui PRIVATE -O2 -Wall)
    target_link_libraries(imager-gui pthread m dl)
endif()

# Install target
install(TARGETS imager imager-gui DESTINATION bin)