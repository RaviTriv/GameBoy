#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

class RAM;
class CPU;
class PPU;
class LCD;
class StateSerializer {
 public:
  StateSerializer(CPU &cpu, RAM &ram, PPU &ppu, LCD &lcd);
  [[nodiscard]] bool save(std::ostream &out, const std::string &label = {});
  [[nodiscard]] bool load(std::istream &in);
  [[nodiscard]] bool saveState(const std::string &title);
  [[nodiscard]] bool loadState(const std::string &title);

 private:
  CPU &cpu;
  RAM &ram;
  PPU &ppu;
  LCD &lcd;

  void saveCPUState(std::ostream &file);
  void saveRAMState(std::ostream &file);
  void savePPUState(std::ostream &file);
  void saveLCDState(std::ostream &file);

  void loadCPUState(std::istream &file);
  void loadRAMState(std::istream &file);
  void loadPPUState(std::istream &file);
  void loadLCDState(std::istream &file);

  std::string removeSpaces(const std::string &str) const;

  static constexpr uint32_t MAX_HEADER_LENGTH = 256;
};
