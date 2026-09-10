cmake_minimum_required(VERSION 3.20)
project(RavenXD-Launcher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT → single exe
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# ---------- ImGui ----------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.5
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

# ---------- miniz ----------
FetchContent_Declare(miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(miniz)

# ---------- nlohmann/json ----------
find_package(nlohmann_json CONFIG REQUIRED)

# ---------- Embed assets/ravenxd.png as C array ----------
if(EXISTS "${CMAKE_SOURCE_DIR}/assets/ravenxd.png")
    file(READ "${CMAKE_SOURCE_DIR}/assets/ravenxd.png" RAVENXD_LOGO_HEX HEX)
    string(REGEX REPLACE "(..)" "0x\\1," RAVENXD_LOGO_BYTES "${RAVENXD_LOGO_HEX}")
else()
    set(RAVENXD_LOGO_BYTES "0x00")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/embedded_assets.h"
"#pragma once
static const unsigned char RAVENXD_LOGO_PNG[] = { ${RAVENXD_LOGO_BYTES} };
static const unsigned int  RAVENXD_LOGO_PNG_SIZE = sizeof(RAVENXD_LOGO_PNG);
")

add_executable(RavenXD-Launcher WIN32
    src/main.cpp
    src/Launcher.cpp
    src/DownloadManager.cpp
    src/Minecraft.cpp
    src/ModManager.cpp
    src/Settings.cpp
    src/UI/UI.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(RavenXD-Launcher PRIVATE
    src
    ${CMAKE_BINARY_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${miniz_SOURCE_DIR}
)

target_link_libraries(RavenXD-Launcher PRIVATE
    nlohmann_json::nlohmann_json
    miniz
    d3d11 dxgi d3dcompiler dwmapi winhttp shlwapi version ole32 comctl32 shell32
)

set_target_properties(RavenXD-Launcher PROPERTIES
    OUTPUT_NAME "RavenXD-Launcher"
    VS_DPI_AWARE "PerMonitor"
)