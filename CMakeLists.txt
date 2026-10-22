cmake_minimum_required(VERSION 3.30)

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER g++-16)
endif()

project(GameBoy LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(BUILD_APP "Build UI" ON)
option(BUILD_TESTS "Build headless for testing" ON)
option(FETCH_TEST_ROMS "Download extra test ROM suites" ON)
option(GB_SANITIZE "Build with -fsanitize=address,undefined" OFF)
set(GB_TEST_TIMEOUT 120 CACHE STRING "Per-test ctest TIMEOUT in seconds")
set(GB_CONTRACTS "" CACHE STRING
    "Contract evaluation semantic: ignore, observe, enforce, quick_enforce (empty = auto)")
set_property(CACHE GB_CONTRACTS PROPERTY STRINGS "" ignore observe enforce quick_enforce)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

enable_testing()

set(GB_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wimplicit-fallthrough
    -Wconversion -Wsign-conversion -Werror)

if(GB_CONTRACTS STREQUAL "")
    if(BUILD_APP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(GB_CONTRACT_SEMANTIC ignore)
    else()
        set(GB_CONTRACT_SEMANTIC enforce)
    endif()
elseif(GB_CONTRACTS MATCHES "^(ignore|observe|enforce|quick_enforce)$")
    set(GB_CONTRACT_SEMANTIC ${GB_CONTRACTS})
else()
    message(FATAL_ERROR "GB_CONTRACTS must be ignore, observe, enforce or quick_enforce")
endif()
message(STATUS "Contract evaluation semantic: ${GB_CONTRACT_SEMANTIC}")

if(GB_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

set(CORE_SOURCES
    src/lib/apu/Apu.cpp
    src/lib/apu/Channel.cpp
    src/lib/Bus.cpp
    src/lib/cartridge/Cartridge.cpp
    src/lib/cartridge/Mbc.cpp
    src/lib/cpu/Cpu.cpp
    src/lib/FramePacer.cpp
    src/lib/Gamepad.cpp
    src/lib/Io.cpp
    src/lib/Machine.cpp
    src/lib/ppu/Dma.cpp
    src/lib/ppu/Lcd.cpp
    src/lib/ppu/Pipeline.cpp
    src/lib/ppu/Ppu.cpp
    src/lib/Ram.cpp
    src/lib/StateSerializer.cpp
    src/lib/StateSerializerFile.cpp
    src/lib/Timer.cpp
)

find_package(spdlog 1.11 REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)

add_library(gameboy_core OBJECT ${CORE_SOURCES})

target_include_directories(gameboy_core PUBLIC include src)

target_link_libraries(gameboy_core PUBLIC spdlog::spdlog_header_only fmt::fmt-header-only)

target_compile_features(gameboy_core PUBLIC cxx_std_26)
target_compile_options(gameboy_core PUBLIC -freflection -fcontracts
    -fcontract-evaluation-semantic=${GB_CONTRACT_SEMANTIC})
target_link_options(gameboy_core PUBLIC -fcontracts)

target_compile_options(gameboy_core PRIVATE ${GB_WARNINGS})

if(BUILD_APP)
    find_package(SDL3 3.2 CONFIG REQUIRED)
    find_package(SDL3_ttf 3.2 CONFIG REQUIRED)

    add_executable(gameboy
        src/lib/Gameboy.cpp
        src/lib/Ui.cpp
        src/main.cpp
    )

    target_link_libraries(gameboy PRIVATE
        gameboy_core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
    )

    target_compile_options(gameboy PRIVATE ${GB_WARNINGS})
endif()

if(BUILD_TESTS)
    add_executable(gameboy_tests tests/test_runner.cpp)

    target_link_libraries(gameboy_tests PRIVATE gameboy_core)

    target_compile_options(gameboy_tests PRIVATE ${GB_WARNINGS})

    include(cmake/TestRoms.cmake)
endif()
