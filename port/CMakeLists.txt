cmake_minimum_required(VERSION 3.21)
project(PartnersInTimePort C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

include(FetchContent)

set(SDL2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SDL2_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2_STATIC ON CACHE BOOL "" FORCE)
set(SDL2_MAIN_HANDLED ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2)

# Dear ImGui. The repository root has no CMakeLists, so FetchContent only
# downloads it and the target below lists the sources explicitly.
set(IMGUI_TAG v1.90.9)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG ${IMGUI_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
if(TARGET SDL2::SDL2-static)
  target_link_libraries(imgui PUBLIC SDL2::SDL2-static)
else()
  target_link_libraries(imgui PUBLIC SDL2::SDL2)
endif()

if(WIN32)
  target_link_libraries(imgui PUBLIC opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()

add_library(pit_core STATIC
  src/core/pit_archive.c
  src/core/pit_gfx.c
  src/core/pit_ingest.c
  src/core/pit_mods.c
  src/core/pit_nds_banner.c
  src/core/pit_patch_data.h
  src/core/pit_patcher.c
  src/core/pit_png.c
  src/core/pit_rom.c
  src/core/pit_sha1.c
  src/core/pit_video.c
)
target_include_directories(pit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Headless driver: proves the plan still produces the expected bytes without
# needing a window, so the packaging script and the round-trip test can use it.
add_executable(pit_patch src/patcher_main.c)
target_link_libraries(pit_patch PRIVATE pit_core)

add_library(pit_platform_sdl2 STATIC
  src/platform/sdl2/pit_platform_sdl2.c
)
target_include_directories(pit_platform_sdl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(pit_platform_sdl2 PUBLIC pit_core)

if(TARGET SDL2::SDL2-static)
  target_link_libraries(pit_platform_sdl2 PUBLIC SDL2::SDL2-static)
else()
  target_link_libraries(pit_platform_sdl2 PUBLIC SDL2::SDL2)
endif()

add_executable(pit src/main.c)
target_link_libraries(pit PRIVATE pit_platform_sdl2)

# Graphical patcher. Shares pit_core with the headless driver, so both apply the
# same generated plan and cannot drift apart.
add_executable(pit_patcher
  src/patcher_ui_main.c
  src/platform/sdl2/pit_patcher_ui.c
)
target_link_libraries(pit_patcher PRIVATE pit_platform_sdl2 pit_core)
target_include_directories(pit_patcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Launcher. A C++ Dear ImGui front end over the same pit_core the headless
# driver and the SDL patcher use, so every front end applies the same plan.
add_executable(pit_launcher
  src/ui/launcher_main.cpp
  src/ui/launcher_theme.cpp
  src/ui/launcher_config.cpp
)
target_link_libraries(pit_launcher PRIVATE pit_core imgui)
target_include_directories(pit_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
find_package(Threads REQUIRED)
target_link_libraries(pit_launcher PRIVATE Threads::Threads)
if(WIN32)
  # GetOpenFileName for the browse buttons and ShellExecute for "open folder".
  target_link_libraries(pit_launcher PRIVATE comdlg32 shell32 ole32)
endif()

if(WIN32)
  # GetOpenFileName for the Browse buttons.
  target_link_libraries(pit_patcher PRIVATE comdlg32)
endif()

install(TARGETS pit pit_patch pit_patcher pit_launcher RUNTIME DESTINATION bin)
