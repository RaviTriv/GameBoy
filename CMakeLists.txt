cmake_minimum_required(VERSION 3.30)

project(GameBoy LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_APP "Build UI" ON)
option(BUILD_TESTS "Build headless for testing" ON)
option(FETCH_TEST_ROMS "Download extra test ROM suites" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

enable_testing()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(GB_WARNINGS -Wall -Wextra -Wpedantic -Werror)
endif()

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
    target_compile_options(gameboy_core PRIVATE ${GB_WARNINGS})
endif()

if(BUILD_APP)
    find_package(SDL3 3.2 CONFIG REQUIRED)
    find_package(SDL3_ttf 3.2 CONFIG REQUIRED)

    add_executable(gameboy
        src/lib/Gameboy.cpp
        src/lib/FramePacer.cpp
        src/lib/StateSerializer.cpp
        src/lib/StateSerializerFile.cpp
        src/lib/Ui.cpp
        src/main.cpp
    )

    target_link_libraries(gameboy PRIVATE
        gameboy_core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(gameboy PRIVATE ${GB_WARNINGS})
    endif()
endif()

if(BUILD_TESTS)
    add_executable(gameboy_tests tests/test_runner.cpp)

    target_link_libraries(gameboy_tests PRIVATE gameboy_core)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(gameboy_tests PRIVATE ${GB_WARNINGS})
    endif()

    file(GLOB BLARGG_ROMS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/roms/blargg/*.gb")
    foreach(rom ${BLARGG_ROMS})
        get_filename_component(rom_name "${rom}" NAME_WE)
        add_test(NAME "blargg.${rom_name}"
                 COMMAND gameboy_tests "${rom}")
        set_tests_properties("blargg.${rom_name}" PROPERTIES TIMEOUT 120 LABELS "cpu")
    endforeach()

    if(FETCH_TEST_ROMS)
        include(FetchContent)
        FetchContent_Declare(gb_test_roms
            URL      https://github.com/c-sp/game-boy-test-roms/releases/download/v7.0/game-boy-test-roms-v7.0.zip
            URL_HASH SHA256=b9a9d7a1075aa35a3d07c07c34974048672d8520dca9e07a50178f5860c3832c
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(gb_test_roms)

        set(BLARGG_DIR "${gb_test_roms_SOURCE_DIR}/blargg")

        function(add_rom_test name rom label)
            add_test(NAME "${name}" COMMAND gameboy_tests "${rom}")
            set_tests_properties("${name}" PROPERTIES TIMEOUT 120 LABELS "${label}")
        endfunction()

        # Not yet passing -- uncomment as each is fixed.
        # add_rom_test(blargg.halt_bug      "${BLARGG_DIR}/halt_bug.gb"                 cpu)
        # add_rom_test(blargg.instr_timing  "${BLARGG_DIR}/instr_timing/instr_timing.gb" cpu)
        # add_rom_test(blargg.mem_timing    "${BLARGG_DIR}/mem_timing/mem_timing.gb"     cpu)
        # add_rom_test(blargg.mem_timing_2  "${BLARGG_DIR}/mem_timing-2/mem_timing.gb"   cpu)
        # add_rom_test(blargg.oam_bug       "${BLARGG_DIR}/oam_bug/oam_bug.gb"           ppu)
    endif()
endif()
