if(NOT FETCH_TEST_ROMS)
    message(WARNING "FETCH_TEST_ROMS is OFF: no test ROMs are registered, so ctest will run nothing")
    return()
endif()

include(FetchContent)
FetchContent_Declare(gb_test_roms
    URL      https://github.com/c-sp/game-boy-test-roms/releases/download/v7.0/game-boy-test-roms-v7.0.zip
    URL_HASH SHA256=b9a9d7a1075aa35a3d07c07c34974048672d8520dca9e07a50178f5860c3832c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(gb_test_roms)

set(GB_ROMS "${gb_test_roms_SOURCE_DIR}")
set(GB_SIGNATURE_BUDGET 10)
set(GB_HRAM_FRAMES 30)

set(GB_BROKEN_TEST_ROMS
    blargg/oam_bug/rom_singles/7-timing_effect.gb
    gbmicrotest/halt_op_dupe_delay.gb
    gbmicrotest/stat_write_glitch_l154_d.gb
)

# gb_rom_test(<name> <rom> [ARGS ...] [LABELS ...] [PASS_HASH <hash>])
function(gb_rom_test name rom)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "" "PASS_HASH" "ARGS;LABELS")
    file(RELATIVE_PATH rel "${GB_ROMS}" "${rom}")
    if(rel IN_LIST GB_BROKEN_TEST_ROMS)
        return()
    endif()
    add_test(NAME "${name}" COMMAND gameboy_tests "${rom}" ${ARG_ARGS})
    set_tests_properties("${name}" PROPERTIES TIMEOUT ${GB_TEST_TIMEOUT} LABELS "${ARG_LABELS}")
    if(ARG_PASS_HASH)
        set_tests_properties("${name}" PROPERTIES PASS_REGULAR_EXPRESSION "^${ARG_PASS_HASH}")
    endif()
endfunction()

# gb_test_id(<out> <relpath>): "timer/tim00.gb" -> "timer.tim00"
function(gb_test_id out relpath)
    string(REGEX REPLACE "\\.gbc?$" "" id "${relpath}")
    string(REPLACE "/" "." id "${id}")
    set(${out} "${id}" PARENT_SCOPE)
endfunction()

# gb_area(<out> <relpath> <default> [<regex> <area>]...): first matching regex wins.
function(gb_area out relpath default)
    string(TOLOWER "${relpath}" path)
    set(rules ${ARGN})
    list(LENGTH rules count)
    if(count GREATER 0)
        math(EXPR last "${count} - 1")
        foreach(i RANGE 0 ${last} 2)
            math(EXPR j "${i} + 1")
            list(GET rules ${i} pattern)
            list(GET rules ${j} area)
            if(path MATCHES "${pattern}")
                set(${out} "${area}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endif()
    set(${out} "${default}" PARENT_SCOPE)
endfunction()

set(GB_MOONEYE_AREA_RULES
    "^(ppu|gpu)/"            ppu
    "^timer/|^div_timing"    timer
    "^oam_dma"               dma
    "^serial/"               serial
    "^interrupts/|^(ei_|di_|rapid_di_ei|halt_|intr_timing|reti_|if_ie_registers)" interrupts
    "^boot_"                 boot
    "unused_hwio"            io
    "mem_oam"                ppu
)

# Mooneye model suffixes that describe a DMG (no suffix also counts).
set(GB_MOONEYE_DMG_SUFFIX "^(G|GS|dmgABC|dmgABCmgb|dmgABCmgbS)$")

function(gb_mooneye_runs_on_dmg out rom)
    get_filename_component(stem "${rom}" NAME_WE)
    set(${out} TRUE PARENT_SCOPE)
    if(stem MATCHES "-([A-Za-z0-9]+)$")
        if(NOT CMAKE_MATCH_1 MATCHES "${GB_MOONEYE_DMG_SUFFIX}")
            set(${out} FALSE PARENT_SCOPE)
        endif()
    endif()
endfunction()

# --- Blargg ---------------------------------------------------------------

set(BLARGG_DIR "${GB_ROMS}/blargg")

file(GLOB roms "${BLARGG_DIR}/cpu_instrs/individual/*.gb")
foreach(rom ${roms})
    get_filename_component(stem "${rom}" NAME_WE)
    gb_rom_test("blargg.${stem}" "${rom}" LABELS blargg cpu)
endforeach()

gb_rom_test(blargg.instr_timing "${BLARGG_DIR}/instr_timing/instr_timing.gb" LABELS blargg cpu)
gb_rom_test(blargg.halt_bug "${BLARGG_DIR}/halt_bug.gb"
    ARGS --frames 180 --hash LABELS blargg cpu PASS_HASH 5c09de1ce5732c7c)

# gb_blargg_suite(<id> <combined rom> <singles dir> <area> [protocol args...])
function(gb_blargg_suite id combined singles area)
    gb_rom_test("blargg.${id}" "${BLARGG_DIR}/${combined}" ARGS ${ARGN} LABELS blargg ${area})
    file(GLOB roms "${BLARGG_DIR}/${singles}/*.gb")
    foreach(rom ${roms})
        get_filename_component(stem "${rom}" NAME_WE)
        gb_rom_test("blargg.${id}.${stem}" "${rom}" ARGS ${ARGN} LABELS blargg ${area})
    endforeach()
endfunction()

gb_blargg_suite(mem_timing   mem_timing/mem_timing.gb   mem_timing/individual    cpu)
gb_blargg_suite(mem_timing_2 mem_timing-2/mem_timing.gb mem_timing-2/rom_singles cpu --memory)
gb_blargg_suite(oam_bug      oam_bug/oam_bug.gb         oam_bug/rom_singles      ppu --memory)
gb_blargg_suite(dmg_sound    dmg_sound/dmg_sound.gb     dmg_sound/rom_singles    apu --memory)

# --- Mooneye (Gekkio) -----------------------------------------------------

set(MOONEYE_DIR "${GB_ROMS}/mooneye-test-suite")

file(GLOB_RECURSE roms RELATIVE "${MOONEYE_DIR}/acceptance" "${MOONEYE_DIR}/acceptance/*.gb")
foreach(rel ${roms})
    gb_mooneye_runs_on_dmg(dmg "${rel}")
    if(dmg)
        gb_test_id(id "${rel}")
        gb_area(area "${rel}" cpu ${GB_MOONEYE_AREA_RULES})
        gb_rom_test("mooneye.${id}" "${MOONEYE_DIR}/acceptance/${rel}"
            ARGS ${GB_SIGNATURE_BUDGET} --regs LABELS mooneye ${area})
    endif()
endforeach()

file(GLOB_RECURSE roms RELATIVE "${MOONEYE_DIR}/emulator-only" "${MOONEYE_DIR}/emulator-only/*.gb")
foreach(rel ${roms})
    gb_test_id(id "${rel}")
    gb_rom_test("mooneye.${id}" "${MOONEYE_DIR}/emulator-only/${rel}"
        ARGS ${GB_SIGNATURE_BUDGET} --regs LABELS mooneye mbc)
endforeach()

# --- Mooneye (wilbertpol) -------------------------------------------------

set(WILBERTPOL_DIR "${GB_ROMS}/mooneye-test-suite-wilbertpol")

file(GLOB_RECURSE roms RELATIVE "${WILBERTPOL_DIR}/acceptance" "${WILBERTPOL_DIR}/acceptance/*.gb")
foreach(rel ${roms})
    gb_mooneye_runs_on_dmg(dmg "${rel}")
    if(dmg)
        gb_test_id(id "${rel}")
        gb_area(area "${rel}" cpu ${GB_MOONEYE_AREA_RULES})
        gb_rom_test("wilbertpol.${id}" "${WILBERTPOL_DIR}/acceptance/${rel}"
            ARGS ${GB_SIGNATURE_BUDGET} --regs LABELS wilbertpol ${area})
    endif()
endforeach()

file(GLOB roms "${WILBERTPOL_DIR}/emulator-only/*.gb")
foreach(rom ${roms})
    get_filename_component(stem "${rom}" NAME_WE)
    gb_rom_test("wilbertpol.${stem}" "${rom}"
        ARGS ${GB_SIGNATURE_BUDGET} --regs LABELS wilbertpol mbc)
endforeach()

# --- AGE (register signature tests; screenshot ones are in screenshot_tests.txt)

set(AGE_DIR "${GB_ROMS}/age-test-roms")

file(GLOB_RECURSE roms RELATIVE "${AGE_DIR}" "${AGE_DIR}/*.gb")
foreach(rel ${roms})
    get_filename_component(stem "${rel}" NAME_WE)
    if(stem MATCHES "(^|-)dmg")
        gb_test_id(id "${rel}")
        gb_area(area "${rel}" ppu "^halt/" interrupts)
        gb_rom_test("age.${id}" "${AGE_DIR}/${rel}"
            ARGS ${GB_SIGNATURE_BUDGET} --regs LABELS age ${area})
    endif()
endforeach()

# --- GBMicrotest ----------------------------------------------------------

# ROMs that never write a result to $FF82: manual checks and testbenches.
set(GBMICROTEST_NOT_SELF_CHECKING
    000-oam_lock 000-write_to_x8000 001-vram_unlocked 002-vram_locked
    004-tima_boot_phase 004-tima_cycle_timer 007-lcd_on_stat 400-dma
    500-scx-timing 800-ppu-latch-scx 801-ppu-latch-scy 802-ppu-latch-tileselect
    803-ppu-latch-bgdisplay audio_testbench cpu_bus_1 dma_basic flood_vram
    lcdon_write_timing ly_while_lcd_off minimal mode2_stat_int_to_oam_unlock
    oam_sprite_trashing poweron ppu_scx_vs_bgp ppu_sprite_testbench
    ppu_spritex_vs_scx ppu_win_vs_wx ppu_wx_early temp toggle_lcdc
    wave_write_to_0xC003
)

file(GLOB roms "${GB_ROMS}/gbmicrotest/*.gb")
foreach(rom ${roms})
    get_filename_component(stem "${rom}" NAME_WE)
    if(NOT stem IN_LIST GBMICROTEST_NOT_SELF_CHECKING)
        gb_area(area "${stem}" ppu
            "^(timer_|div_)"   timer
            "^poweron"         boot
            "^(int_|is_if_set)" interrupts
            "^dma"             dma
            "^(audio|wave_)"   apu
            "^mbc"             mbc
            "^(cpu_bus|halt_bug|halt_op_dupe)" cpu)
        gb_rom_test("gbmicrotest.${stem}" "${rom}"
            ARGS --ff82 --frames ${GB_HRAM_FRAMES} LABELS gbmicrotest ${area})
    endif()
endforeach()

# --- Screenshot tests (tests/gen_screenshot_tests.py writes the list) ----

set(GB_SCREENSHOT_LABEL_dmg-acid2 acid2)
set(GB_SCREENSHOT_LABEL_strikethrough strikethrough)
set(GB_SCREENSHOT_LABEL_little-things-gb little-things)
set(GB_SCREENSHOT_LABEL_mooneye-test-suite mooneye)
set(GB_SCREENSHOT_LABEL_scribbltests scribbltests)
set(GB_SCREENSHOT_LABEL_turtle-tests turtle)
set(GB_SCREENSHOT_LABEL_mealybug-tearoom-tests mealybug)
set(GB_SCREENSHOT_LABEL_age-test-roms age)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/tests/screenshot_tests.txt" lines)
foreach(line ${lines})
    string(REPLACE "\t" ";" fields "${line}")
    list(GET fields 0 rel)
    list(GET fields 1 mode)
    list(GET fields 2 frames)
    list(GET fields 3 hash)
    string(REGEX MATCH "^[^/]+" dir "${rel}")
    string(REGEX REPLACE "^[^/]+/" "" inner "${rel}")
    set(suite "${GB_SCREENSHOT_LABEL_${dir}}")
    if(NOT suite)
        message(FATAL_ERROR "screenshot_tests.txt: no suite label for ${dir}")
    endif()
    gb_test_id(id "${inner}")
    set(args --frames ${frames} --hash)
    if(mode STREQUAL "break")
        list(PREPEND args --until-break)
    endif()
    gb_rom_test("${suite}.${id}" "${GB_ROMS}/${rel}" ARGS ${args} LABELS ${suite} ppu PASS_HASH ${hash})
endforeach()

# --- Hash, input and save-state tests -------------------------------------

gb_rom_test(mbc3_tester "${GB_ROMS}/mbc3-tester/mbc3-tester.gb"
    ARGS --frames 60 --hash LABELS mbc3-tester mbc PASS_HASH 694d5c9493d33997)

set(RTC3TEST_ROM "${GB_ROMS}/rtc3test/rtc3test.gb")
gb_rom_test(rtc3test.basic "${RTC3TEST_ROM}"
    ARGS --frames 1000 --input 120:A --hash LABELS rtc3test mbc PASS_HASH e2beae8fcd88e055)
gb_rom_test(rtc3test.range "${RTC3TEST_ROM}"
    ARGS --frames 700 --input 120:D,140:A --hash LABELS rtc3test mbc PASS_HASH 12ccb1439ce7fca4)
gb_rom_test(rtc3test.sub_second_writes "${RTC3TEST_ROM}"
    ARGS --frames 1900 --input 120:D,140:D,160:A --hash LABELS rtc3test mbc PASS_HASH 021c5fcd50013455)

gb_rom_test(savestate.cpu_instrs "${BLARGG_DIR}/cpu_instrs/cpu_instrs.gb"
    ARGS --savestate LABELS savestate)
gb_rom_test(savestate.dmg_acid2 "${GB_ROMS}/dmg-acid2/dmg-acid2.gb"
    ARGS --savestate-line 72 LABELS savestate)
