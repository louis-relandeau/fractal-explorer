cmake_minimum_required(VERSION 3.10)
project(fractal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)
find_package(yaml-cpp REQUIRED)
find_package(OpenMP REQUIRED)

add_executable(fractal 
    main.cpp
    src/ConfigLoader.cpp
    src/Mandelbrot.cpp
    src/EventHandler.cpp
    src/FractalBase.cpp
)

target_include_directories(fractal PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(fractal PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    yaml-cpp::yaml-cpp
    OpenMP::OpenMP_CXX
)

add_custom_target(profile
    COMMAND valgrind --tool=callgrind --callgrind-out-file=callgrind.out.%p ./fractal
    COMMAND kcachegrind callgrind.out
    DEPENDS fractal
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)