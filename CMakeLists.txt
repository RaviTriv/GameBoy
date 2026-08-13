cmake_minimum_required(VERSION 3.30)

project(GameBoy LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_APP "Build UI" ON)
option(BUILD_TESTS "Build headless for testing" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

enable_testing()

set(CORE_SOURCES
    src/lib/apu/Apu.cpp
    src/lib/apu/Channel.cpp
    src/lib/Bus.cpp
    src/lib/cartridge/Cartridge.cpp
    src/lib/cartridge/Mbc.cpp
    src/lib/cpu/Cpu.cpp
    src/lib/Gamepad.cpp
    src/lib/Io.cpp
    src/lib/ppu/Dma.cpp
    src/lib/ppu/Lcd.cpp
    src/lib/ppu/Pipeline.cpp
    src/lib/ppu/PixelFifo.cpp
    src/lib/ppu/Ppu.cpp
    src/lib/Ram.cpp
    src/lib/Timer.cpp
)

find_package(spdlog 1.11 REQUIRED CONFIG)

add_library(gameboy_core OBJECT ${CORE_SOURCES})

target_include_directories(gameboy_core PUBLIC include src)

target_link_libraries(gameboy_core PUBLIC spdlog::spdlog)

target_compile_features(gameboy_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(gameboy_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_APP)
    find_package(SDL3 3.2 CONFIG REQUIRED)
    find_package(SDL3_ttf 3.2 CONFIG REQUIRED)

    add_executable(gameboy
        src/lib/Gameboy.cpp
        src/lib/FramePacer.cpp
        src/lib/StateSerializer.cpp
        src/lib/Ui.cpp
        src/main.cpp
    )

    target_link_libraries(gameboy PRIVATE
        gameboy_core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(gameboy PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(BUILD_TESTS)
    add_executable(gameboy_tests tests/test_runner.cpp)

    target_link_libraries(gameboy_tests PRIVATE gameboy_core)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(gameboy_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    file(GLOB BLARGG_ROMS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/roms/blargg/*.gb")
    foreach(rom ${BLARGG_ROMS})
        get_filename_component(rom_name "${rom}" NAME_WE)
        add_test(NAME "blargg.${rom_name}"
                 COMMAND gameboy_tests "${rom}")
        set_tests_properties("blargg.${rom_name}" PROPERTIES TIMEOUT 120 LABELS "cpu")
    endforeach()
endif()
