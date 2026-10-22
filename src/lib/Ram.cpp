#include "Ram.h"

#include <cstdint>
#include <istream>
#include <ostream>

#include "Serializable.h"

uint8_t RAM::readWRAM(uint16_t address) const {
  return state.wram[address - WRAM_BASE];
}

void RAM::writeWRAM(uint16_t address, uint8_t value) {
  state.wram[address - WRAM_BASE] = value;
}

uint8_t RAM::readHRAM(uint16_t address) const {
  return state.hram[address - HRAM_BASE];
}

void RAM::writeHRAM(uint16_t address, uint8_t value) {
  state.hram[address - HRAM_BASE] = value;
}

void RAM::save(std::ostream& out) const { serial::saveFields(out, *this); }

void RAM::load(std::istream& in) { serial::loadFields(in, *this); }
