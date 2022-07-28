cmake_minimum_required(VERSION 3.0.0)
project(fafmoholog VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CXX_EXTENSIONS NO)

add_subdirectory("kthook")

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_library(imgui STATIC)

target_include_directories(imgui
    PUBLIC ${IMGUI_DIR}
    PUBLIC ${IMGUI_DIR}/backends
)
target_sources(imgui PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    # ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/backends/imgui_impl_dx9.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)
#ImGui

include_directories("$ENV{DXSDK_DIR}/Include")
#link_directories("$ENV{DXSDK_DIR}/Lib/x86")

add_library(${PROJECT_NAME} SHARED "fafmoholog.cpp")

set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".asi")

target_link_libraries(${PROJECT_NAME} PRIVATE kthook imgui)