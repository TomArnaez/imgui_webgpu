cmake_minimum_required(VERSION 3.30.0)
project(app VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

add_executable(app application.cpp)

find_package(glfw3 CONFIG REQUIRED)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.5
)

FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_wgpu.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})

add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

target_link_libraries(imgui PRIVATE webgpu glfw)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(app PRIVATE webgpu glfw glfw3webgpu imgui)
target_compile_features(app PRIVATE cxx_std_23)
