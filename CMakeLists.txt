cmake_minimum_required(VERSION 3.10)
project(fractal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(yaml-cpp REQUIRED)

add_executable(fractal src/main.cpp)

target_link_libraries(fractal PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    yaml-cpp
)