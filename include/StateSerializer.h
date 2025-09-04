#pragma once

#include <string>
#include <fstream>
#include <memory>

class RAM;
class CPU;
class PPU;
class LCD;
class StateSerializer
{
public:
  StateSerializer(std::shared_ptr<CPU> cpu, std::shared_ptr<RAM> ram, std::shared_ptr<PPU> ppu, std::shared_ptr<LCD> lcd);
  bool saveState(const std::string &title);
  bool loadState(const std::string &title);

private:
  std::shared_ptr<CPU> cpu;
  std::shared_ptr<RAM> ram;
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<LCD> lcd;

  void saveCPUState(std::ofstream &file);
  void saveRAMState(std::ofstream &file);
  void savePPUState(std::ofstream &file);
  void saveLCDState(std::ofstream &file);

  void loadCPUState(std::ifstream &file);
  void loadRAMState(std::ifstream &file);
  void loadPPUState(std::ifstream &file);
  void loadLCDState(std::ifstream &file);

  std::string removeSpaces(const std::string &str);
};