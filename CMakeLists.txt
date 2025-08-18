cmake_minimum_required(VERSION 3.30)

project(GameBoy LANGUAGES CXX)

find_package(spdlog REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gameboy
    src/main.cpp
    src/lib/Gameboy.cpp
    src/lib/cartridge/Cartridge.cpp
    src/lib/cartridge/Mbc.cpp
)

target_include_directories(gameboy PRIVATE
    src
)

target_link_libraries(gameboy PRIVATE
PRIVATE spdlog::spdlog_header_only
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gameboy PRIVATE -Wall -Wextra -Wpedantic -DSDLTTF_SAMPLES=ON -g)
endif()