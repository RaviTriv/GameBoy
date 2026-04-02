#pragma once

#include <string>
#include <fstream>

class RAM;
class CPU;
class PPU;
class LCD;
class StateSerializer
{
public:
  StateSerializer(CPU &cpu, RAM &ram, PPU &ppu, LCD &lcd);
  [[nodiscard]] bool saveState(const std::string &title);
  [[nodiscard]] bool loadState(const std::string &title);

private:
  CPU &cpu;
  RAM &ram;
  PPU &ppu;
  LCD &lcd;

  void saveCPUState(std::ofstream &file);
  void saveRAMState(std::ofstream &file);
  void savePPUState(std::ofstream &file);
  void saveLCDState(std::ofstream &file);

  void loadCPUState(std::ifstream &file);
  void loadRAMState(std::ifstream &file);
  void loadPPUState(std::ifstream &file);
  void loadLCDState(std::ifstream &file);

  std::string removeSpaces(const std::string &str) const;
};