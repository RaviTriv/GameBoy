cmake_minimum_required(VERSION 3.30)

project(GameBoy LANGUAGES CXX)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(spdlog REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gameboy
    src/lib/Bus.cpp
    src/lib/cartridge/Cartridge.cpp
    src/lib/cartridge/Mbc.cpp
    src/lib/cpu/Cpu.cpp
    src/lib/cpu/InstructionsDecoder.cpp
    src/lib/cpu/InstructionsExecuter.cpp
    src/lib/cpu/Interrupts.cpp
    src/lib/Gameboy.cpp
    src/lib/Io.cpp
    src/lib/ppu/Ppu.cpp
    src/lib/ppu/Lcd.cpp
    src/lib/Ram.cpp
    src/lib/Timer.cpp
    src/lib/Ui.cpp
    src/main.cpp
)

target_include_directories(gameboy PRIVATE
    src
    ${SDL3_INCLUDE_DIRS}
    ${SDL3_TTF_INCLUDE_DIRS} 
)

target_link_libraries(gameboy PRIVATE
SDL3::SDL3 
SDL3_ttf::SDL3_ttf
PRIVATE spdlog::spdlog_header_only
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gameboy PRIVATE -Wall -Wextra -Wpedantic -DSDLTTF_SAMPLES=ON -g)
endif()