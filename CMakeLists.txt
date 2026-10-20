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

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

enable_testing()

set(GB_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wimplicit-fallthrough
    -Wconversion -Wsign-conversion -Werror)

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
    src/lib/ppu/PixelFifo.cpp
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
target_compile_options(gameboy_core PUBLIC -freflection)

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

    file(GLOB BLARGG_ROMS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/roms/blargg/*.gb")
    foreach(rom ${BLARGG_ROMS})
        get_filename_component(rom_name "${rom}" NAME_WE)
        add_test(NAME "blargg.${rom_name}"
                 COMMAND gameboy_tests "${rom}")
        set_tests_properties("blargg.${rom_name}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "cpu")
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
        set(MOONEYE_DIR "${gb_test_roms_SOURCE_DIR}/mooneye-test-suite/acceptance")
        set(MOONEYE_EMULATOR_ONLY_DIR "${gb_test_roms_SOURCE_DIR}/mooneye-test-suite/emulator-only")
        set(ACID2_ROM "${gb_test_roms_SOURCE_DIR}/dmg-acid2/dmg-acid2.gb")

        function(add_rom_test name rom label)
            add_test(NAME "${name}" COMMAND gameboy_tests "${rom}")
            set_tests_properties("${name}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "${label}")
        endfunction()

        function(add_mooneye_test relpath)
            string(REPLACE ".gb" "" test_id "${relpath}")
            string(REPLACE "/" "." test_id "${test_id}")
            add_test(NAME "mooneye.${test_id}"
                     COMMAND gameboy_tests "${MOONEYE_DIR}/${relpath}" --mooneye)
            set_tests_properties("mooneye.${test_id}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "mooneye")
        endfunction()

        function(add_blargg_memory_test name rom)
            add_test(NAME "blargg.${name}" COMMAND gameboy_tests "${rom}" --memory)
            set_tests_properties("blargg.${name}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "apu")
        endfunction()

        function(mark_known_fail name)
            set_property(TEST "${name}" APPEND PROPERTY LABELS known_fail)
        endfunction()

        function(add_mooneye_mbc_test relpath)
            string(REPLACE ".gb" "" test_id "${relpath}")
            string(REPLACE "/" "." test_id "${test_id}")
            add_test(NAME "mooneye.${test_id}"
                     COMMAND gameboy_tests "${MOONEYE_EMULATOR_ONLY_DIR}/${relpath}" --mooneye)
            set_tests_properties("mooneye.${test_id}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "mbc")
        endfunction()

        add_mooneye_mbc_test(mbc1/bits_bank1.gb)
        add_mooneye_mbc_test(mbc1/bits_bank2.gb)
        add_mooneye_mbc_test(mbc1/bits_mode.gb)
        add_mooneye_mbc_test(mbc1/bits_ramg.gb)
        add_mooneye_mbc_test(mbc1/ram_64kb.gb)
        add_mooneye_mbc_test(mbc1/ram_256kb.gb)
        add_mooneye_mbc_test(mbc1/rom_512kb.gb)
        add_mooneye_mbc_test(mbc1/rom_1Mb.gb)
        add_mooneye_mbc_test(mbc1/rom_2Mb.gb)
        add_mooneye_mbc_test(mbc1/rom_4Mb.gb)
        add_mooneye_mbc_test(mbc1/rom_8Mb.gb)
        add_mooneye_mbc_test(mbc1/rom_16Mb.gb)
        add_mooneye_mbc_test(mbc1/multicart_rom_8Mb.gb)

        add_mooneye_mbc_test(mbc2/bits_ramg.gb)
        add_mooneye_mbc_test(mbc2/bits_romb.gb)
        add_mooneye_mbc_test(mbc2/bits_unused.gb)
        add_mooneye_mbc_test(mbc2/ram.gb)
        add_mooneye_mbc_test(mbc2/rom_512kb.gb)
        add_mooneye_mbc_test(mbc2/rom_1Mb.gb)
        add_mooneye_mbc_test(mbc2/rom_2Mb.gb)

        add_mooneye_mbc_test(mbc5/rom_512kb.gb)
        add_mooneye_mbc_test(mbc5/rom_1Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_2Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_4Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_8Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_16Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_32Mb.gb)
        add_mooneye_mbc_test(mbc5/rom_64Mb.gb)

        set(MBC3_TESTER_PASS_HASH 694d5c9493d33997)
        add_test(NAME mbc3_tester
                 COMMAND gameboy_tests "${gb_test_roms_SOURCE_DIR}/mbc3-tester/mbc3-tester.gb" --frames 60 --hash)
        set_tests_properties(mbc3_tester PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS mbc
                             PASS_REGULAR_EXPRESSION "^${MBC3_TESTER_PASS_HASH}")

        set(RTC3TEST_ROM "${gb_test_roms_SOURCE_DIR}/rtc3test/rtc3test.gb")
        function(add_rtc3test name input frames hash)
            add_test(NAME "rtc3test.${name}"
                     COMMAND gameboy_tests "${RTC3TEST_ROM}" --frames ${frames} --input ${input} --hash)
            set_tests_properties("rtc3test.${name}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS mbc
                                 PASS_REGULAR_EXPRESSION "^${hash}")
        endfunction()
        add_rtc3test(basic 120:A 1000 e2beae8fcd88e055)
        add_rtc3test(range 120:D,140:A 700 12ccb1439ce7fca4)
        add_rtc3test(sub_second_writes 120:D,140:D,160:A 1900 021c5fcd50013455)

        add_test(NAME savestate.cpu_instrs
                 COMMAND gameboy_tests "${BLARGG_DIR}/cpu_instrs/cpu_instrs.gb" --savestate)
        set_tests_properties(savestate.cpu_instrs PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS savestate)
        add_test(NAME savestate.dmg_acid2
                 COMMAND gameboy_tests "${ACID2_ROM}" --savestate-line 72)
        set_tests_properties(savestate.dmg_acid2 PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS savestate)

        set(HALT_BUG_PASS_HASH 5c09de1ce5732c7c)
        add_test(NAME blargg.halt_bug
                 COMMAND gameboy_tests "${BLARGG_DIR}/halt_bug.gb" --frames 180 --hash)
        set_tests_properties(blargg.halt_bug PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS cpu
                             PASS_REGULAR_EXPRESSION "^${HALT_BUG_PASS_HASH}")

        # Tests labelled known_fail run and are reported, but never fail CI.
        # Remove the mark_known_fail line once a test passes.
        add_rom_test(blargg.instr_timing  "${BLARGG_DIR}/instr_timing/instr_timing.gb" cpu)
        add_rom_test(blargg.mem_timing    "${BLARGG_DIR}/mem_timing/mem_timing.gb"     cpu)
        add_rom_test(blargg.mem_timing_2  "${BLARGG_DIR}/mem_timing-2/mem_timing.gb"   cpu)
        mark_known_fail(blargg.mem_timing_2)
        add_rom_test(blargg.oam_bug       "${BLARGG_DIR}/oam_bug/oam_bug.gb"           ppu)
        mark_known_fail(blargg.oam_bug)
    endif()
endif()
