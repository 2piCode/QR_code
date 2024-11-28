cmake_minimum_required(VERSION 3.5)

project(qr_code VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (SOURCES src/main.cpp
    src/reed_solomon.cpp
    src/qr_code.cpp
)

add_executable(qr_code ${SOURCES})

target_include_directories(qr_code PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
