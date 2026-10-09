cmake_minimum_required(VERSION 3.16)
project(mandelbrot LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_VERSION VERSION_LESS "3.28")
    include(${CMAKE_SOURCE_DIR}/cmake/ImportSFML.cmake)
else ()
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
    set(SFML_USE_SYSTEM_DEPS OFF)
    FetchContent_MakeAvailable(SFML)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(freetype PRIVATE harfbuzz)
    endif ()
endif ()

file(GLOB SOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(mandelbrot ${SOURCE})
target_link_libraries(mandelbrot PRIVATE
    SFML::Graphics
    SFML::Window
)
