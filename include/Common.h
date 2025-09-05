#pragma once
#include <cstdint>

constexpr uint16_t ROM_START = 0x0000;
constexpr uint16_t ROM_END = 0x7FFF;

constexpr uint16_t VRAM_START = 0x8000;
constexpr uint16_t VRAM_END = 0x9FFF;

constexpr uint16_t CART_RAM_START = 0xA000;
constexpr uint16_t CART_RAM_END = 0xBFFF;

constexpr uint16_t WRAM_START = 0xC000;
constexpr uint16_t WRAM_END = 0xDFFF;

constexpr uint16_t ECHO_RAM_START = 0xE000;
constexpr uint16_t ECHO_RAM_END = 0xFDFF;

constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t OAM_END = 0xFE9F;

constexpr uint16_t UNUSED_START = 0xFEA0;
constexpr uint16_t UNUSED_END = 0xFEFF;

constexpr uint16_t IO_REGISTERS_START = 0xFF00;
constexpr uint16_t IO_REGISTERS_END = 0xFF7F;

constexpr uint16_t HRAM_START = 0xFF80;
constexpr uint16_t HRAM_END = 0xFFFE;

constexpr uint16_t IE_REGISTER = 0xFFFF;

constexpr uint8_t BYTE_MASK = 0xFF;
constexpr uint8_t BYTE_BITS = 8;

constexpr uint16_t JOYPAD_REGISTER = 0xFF00;

constexpr uint16_t SERIAL_DATA_REGISTER = 0xFF01;
constexpr uint16_t SERIAL_CONTROL_REGISTER = 0xFF02;

constexpr uint16_t TIMER_DIV_REGISTER = 0xFF04;
constexpr uint16_t TIMER_COUNTER_REGISTER = 0xFF05;
constexpr uint16_t TIMER_MODULO_REGISTER = 0xFF06;
constexpr uint16_t TIMER_CONTROL_REGISTER = 0xFF07;
constexpr uint16_t TIMER_START = TIMER_DIV_REGISTER;
constexpr uint16_t TIMER_END = TIMER_CONTROL_REGISTER;

constexpr uint16_t IF_REGISTER = 0xFF0F;

constexpr uint16_t APU_START = 0xFF10;
constexpr uint16_t APU_END = 0xFF26;

constexpr uint16_t WAVE_RAM_START = 0xFF30;
constexpr uint16_t WAVE_RAM_END = 0xFF3F;

constexpr uint16_t LCD_START = 0xFF40;
constexpr uint16_t LCD_END = 0xFF4B;

constexpr uint8_t SERIAL_DATA_INDEX = 0;
constexpr uint8_t SERIAL_CONTROL_INDEX = 1;

constexpr int CLOCK_CYCLES = 4;

constexpr int UI_THREAD_SLEEP_MS = 1;

constexpr uint8_t INVALID_READ_VALUE = 0xFF;

constexpr int XRES = 160;
constexpr int YRES = 144;