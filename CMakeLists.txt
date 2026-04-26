cmake_minimum_required(VERSION 3.14)
project(msm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/third_party)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# ---------- FFTW ----------
find_library(FFTW3F_LIB fftw3f REQUIRED)
find_path(FFTW3F_INCLUDE_DIR fftw3.h REQUIRED)

add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
    ${THIRD_PARTY_DIR}/imgui
    ${THIRD_PARTY_DIR}/imgui/backends
)

target_link_libraries(imgui
    PUBLIC
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
)

add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)

target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PUBLIC imgui)

add_executable(main
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/funcs.cpp
)

add_executable(gui
    ${SRC_DIR}/main2.cpp
    ${SRC_DIR}/funcs.cpp
)

target_include_directories(main PRIVATE ${INCLUDE_DIR})

target_include_directories(gui PRIVATE ${INCLUDE_DIR} ${FFTW3F_INCLUDE_DIR})

target_link_libraries(gui
    PRIVATE
    ${FFTW3F_LIB}
    imgui
    implot
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
)

target_link_libraries(main
    PRIVATE
    ${FFTW3F_LIB}
    imgui
    implot
    SDL2::SDL2
    OpenGL::GL
    GLEW::GLEW
)
