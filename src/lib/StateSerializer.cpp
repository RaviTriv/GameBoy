#include "../../include/StateSerializer.h"
#include "../../include/Cpu.h"
#include "../../include/Pipeline.h"
#include "../../include/Ppu.h"
#include "../../include/OamTypes.h"
#include "../../include/Lcd.h"
#include "../../include/Ram.h"
#include "../../include/Logger.h"

#include <ctime>
#include <sstream>
#include <filesystem>
#include <system_error>

StateSerializer::StateSerializer(std::shared_ptr<CPU> cpu, std::shared_ptr<RAM> ram, std::shared_ptr<PPU> ppu, std::shared_ptr<LCD> lcd) : cpu(cpu), ram(ram), ppu(ppu), lcd(lcd)
{
  std::error_code ec;
  if (!std::filesystem::exists("../saves", ec))
  {
    std::filesystem::create_directory("../saves", ec);
    if (ec)
    {
      Logger::GetLogger()->error("Failed to create saves directory: {}", ec.message());
    }
  }
}

std::string StateSerializer::removeSpaces(const std::string &str)
{
  std::string result = str;
  result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
  return result;
}

bool StateSerializer::saveState(const std::string &title)
{
  std::string saveTitle = removeSpaces(title);
  std::filesystem::path savesDir = "../saves";
  std::filesystem::path saveFile = savesDir / (saveTitle + ".sav");
  std::string fileName = saveFile.string();
  std::ofstream file(fileName, std::ios::binary);

  if (!file.is_open())
  {
    Logger::GetLogger()->error("Failed to create save state file: {}", fileName);
    return false;
  }

  try
  {
    std::time_t currentTime = std::time(nullptr);
    std::stringstream headerStream;
    headerStream << saveTitle << "_" << currentTime;
    std::string headerStr = headerStream.str();

    uint32_t headerLength = headerStr.length();
    file.write(reinterpret_cast<const char *>(&headerLength), sizeof(headerLength));
    file.write(headerStr.c_str(), headerStr.length());
    saveCPUState(file);
    saveRAMState(file);
    savePPUState(file);
    saveLCDState(file);
    Logger::GetLogger()->info("State successfully saved to: {}", fileName);
    return true;
  }
  catch (const std::exception &e)
  {
    Logger::GetLogger()->error("Error saving state: {}", e.what());
    return false;
  }
}

void StateSerializer::saveCPUState(std::ofstream &file)
{
  const char *cpuMarker = "CPU_STATE";
  file.write(cpuMarker, 9);

  CPU::State state = cpu->getState();
  file.write(reinterpret_cast<const char *>(&state), sizeof(state));
}

void StateSerializer::saveRAMState(std::ofstream &file)
{
  const char *ramMarker = "RAM_STATE";
  file.write(ramMarker, 9);

  RAM::State state = ram->getState();
  file.write(reinterpret_cast<const char *>(&state), sizeof(state));
}

void StateSerializer::savePPUState(std::ofstream &file)
{
  const char *ppuMarker = "PPU_STATE";
  file.write(ppuMarker, 9);

  const PPU::State &ppuState = ppu->getState();

  file.write(reinterpret_cast<const char *>(&ppuState.currentFrame), sizeof(ppuState.currentFrame));
  file.write(reinterpret_cast<const char *>(&ppuState.lineTicks), sizeof(ppuState.lineTicks));
  file.write(reinterpret_cast<const char *>(&ppuState.windowLine), sizeof(ppuState.windowLine));
  file.write(reinterpret_cast<const char *>(&ppuState.lineSpritesCount), sizeof(ppuState.lineSpritesCount));
  file.write(reinterpret_cast<const char *>(ppuState.vram.data()), ppuState.vram.size());
  file.write(reinterpret_cast<const char *>(ppuState.oamRam.data()), ppuState.oamRam.size() * sizeof(OAM_ENTRY));
  file.write(reinterpret_cast<const char *>(ppuState.videoBuffer.data()), ppuState.videoBuffer.size() * sizeof(uint32_t));

  const Pipeline::State &pipelineState = ppu->getPipelineState();

  file.write(reinterpret_cast<const char *>(&pipelineState.fetchState), sizeof(pipelineState.fetchState));
  file.write(reinterpret_cast<const char *>(pipelineState.pixelFifo.data()), pipelineState.pixelFifo.size() * sizeof(uint32_t));
  file.write(reinterpret_cast<const char *>(&pipelineState.fifoHead), sizeof(pipelineState.fifoHead));
  file.write(reinterpret_cast<const char *>(&pipelineState.fifoTail), sizeof(pipelineState.fifoTail));
  file.write(reinterpret_cast<const char *>(&pipelineState.fifoSize), sizeof(pipelineState.fifoSize));
  file.write(reinterpret_cast<const char *>(&pipelineState.fifoX), sizeof(pipelineState.fifoX));
  file.write(reinterpret_cast<const char *>(&pipelineState.lineX), sizeof(pipelineState.lineX));
  file.write(reinterpret_cast<const char *>(&pipelineState.pushedCount), sizeof(pipelineState.pushedCount));
  file.write(reinterpret_cast<const char *>(&pipelineState.fetchX), sizeof(pipelineState.fetchX));
  file.write(reinterpret_cast<const char *>(pipelineState.bgwBuffer.data()), pipelineState.bgwBuffer.size() * sizeof(uint8_t));
  file.write(reinterpret_cast<const char *>(pipelineState.objectBuffer.data()), pipelineState.objectBuffer.size() * sizeof(uint8_t));
  file.write(reinterpret_cast<const char *>(pipelineState.fetchedEntries.data()), pipelineState.fetchedEntries.size() * sizeof(OAM_ENTRY));
  file.write(reinterpret_cast<const char *>(&pipelineState.mapX), sizeof(pipelineState.mapX));
  file.write(reinterpret_cast<const char *>(&pipelineState.mapY), sizeof(pipelineState.mapY));
  file.write(reinterpret_cast<const char *>(&pipelineState.tileX), sizeof(pipelineState.tileX));
  file.write(reinterpret_cast<const char *>(&pipelineState.tileY), sizeof(pipelineState.tileY));
  file.write(reinterpret_cast<const char *>(&pipelineState.entryCount), sizeof(pipelineState.entryCount));
}

void StateSerializer::saveLCDState(std::ofstream &file)
{
  const char *lcdMarker = "LCD_STATE";
  file.write(lcdMarker, 9);

  const LCD::State &state = lcd->getState();

  file.write(reinterpret_cast<const char *>(&state.lcdc), sizeof(state.lcdc));
  file.write(reinterpret_cast<const char *>(&state.lcds), sizeof(state.lcds));
  file.write(reinterpret_cast<const char *>(&state.scrollX), sizeof(state.scrollX));
  file.write(reinterpret_cast<const char *>(&state.scrollY), sizeof(state.scrollY));
  file.write(reinterpret_cast<const char *>(&state.ly), sizeof(state.ly));
  file.write(reinterpret_cast<const char *>(&state.lyCompare), sizeof(state.lyCompare));
  file.write(reinterpret_cast<const char *>(&state.dma), sizeof(state.dma));

  file.write(reinterpret_cast<const char *>(&state.bgp), sizeof(state.bgp));

  file.write(reinterpret_cast<const char *>(state.palettes.data()),
             state.palettes.size() * sizeof(LCD::PaletteRegister));

  file.write(reinterpret_cast<const char *>(&state.windowX), sizeof(state.windowX));
  file.write(reinterpret_cast<const char *>(&state.windowY), sizeof(state.windowY));

  file.write(reinterpret_cast<const char *>(state.bgColors.data()),
             state.bgColors.size() * sizeof(uint32_t));
  file.write(reinterpret_cast<const char *>(state.ob1Colors.data()),
             state.ob1Colors.size() * sizeof(uint32_t));
  file.write(reinterpret_cast<const char *>(state.ob2Colors.data()),
             state.ob2Colors.size() * sizeof(uint32_t));
}

bool StateSerializer::loadState(const std::string &title)
{
  std::string saveTitle = removeSpaces(title);
  std::filesystem::path savesDir = "../saves";
  std::filesystem::path saveFile = savesDir / (saveTitle + ".sav");
  std::string fileName = saveFile.string();

  std::ifstream file(fileName, std::ios::binary);
  if (!file.is_open())
  {
    Logger::GetLogger()->error("Failed to open save state file: {}", fileName);
    return false;
  }

  try
  {
    uint32_t headerLength;
    file.read(reinterpret_cast<char *>(&headerLength), sizeof(headerLength));

    char *headerBuffer = new char[headerLength];
    file.read(headerBuffer, headerLength);
    delete[] headerBuffer;

    loadCPUState(file);
    loadRAMState(file);
    loadPPUState(file);
    loadLCDState(file);

    Logger::GetLogger()->info("State successfully loaded from: {}", fileName);
    return true;
  }
  catch (const std::exception &e)
  {
    Logger::GetLogger()->error("Error loading state: {}", e.what());
    return false;
  }
}

void StateSerializer::loadCPUState(std::ifstream &file)
{
  char marker[10];
  file.read(marker, 9);
  marker[9] = '\0';

  if (strcmp(marker, "CPU_STATE") != 0)
  {
    throw std::runtime_error("Invalid CPU state marker in save file");
  }

  CPU::State state;
  file.read(reinterpret_cast<char *>(&state), sizeof(state));
  cpu->setState(state);
}

void StateSerializer::loadRAMState(std::ifstream &file)
{
  char marker[10];
  file.read(marker, 9);
  marker[9] = '\0';

  if (strcmp(marker, "RAM_STATE") != 0)
  {
    throw std::runtime_error("Invalid RAM state marker in save file");
  }

  RAM::State state;
  file.read(reinterpret_cast<char *>(&state), sizeof(state));
  ram->setState(state);
}

void StateSerializer::loadPPUState(std::ifstream &file)
{
  char marker[10];
  file.read(marker, 9);
  marker[9] = '\0';

  if (strcmp(marker, "PPU_STATE") != 0)
  {
    throw std::runtime_error("Invalid PPU state marker in save file");
  }

  PPU::State state = ppu->getState();

  file.read(reinterpret_cast<char *>(&state.currentFrame), sizeof(state.currentFrame));
  file.read(reinterpret_cast<char *>(&state.lineTicks), sizeof(state.lineTicks));
  file.read(reinterpret_cast<char *>(&state.windowLine), sizeof(state.windowLine));
  file.read(reinterpret_cast<char *>(&state.lineSpritesCount), sizeof(state.lineSpritesCount));
  file.read(reinterpret_cast<char *>(state.vram.data()), state.vram.size());
  file.read(reinterpret_cast<char *>(state.oamRam.data()), state.oamRam.size() * sizeof(OAM_ENTRY));
  file.read(reinterpret_cast<char *>(state.videoBuffer.data()), state.videoBuffer.size() * sizeof(uint32_t));

  Pipeline::State pipelineState = ppu->getPipelineState();

  file.read(reinterpret_cast<char *>(&pipelineState.fetchState), sizeof(pipelineState.fetchState));
  file.read(reinterpret_cast<char *>(pipelineState.pixelFifo.data()), pipelineState.pixelFifo.size() * sizeof(uint32_t));
  file.read(reinterpret_cast<char *>(&pipelineState.fifoHead), sizeof(pipelineState.fifoHead));
  file.read(reinterpret_cast<char *>(&pipelineState.fifoTail), sizeof(pipelineState.fifoTail));
  file.read(reinterpret_cast<char *>(&pipelineState.fifoSize), sizeof(pipelineState.fifoSize));
  file.read(reinterpret_cast<char *>(&pipelineState.fifoX), sizeof(pipelineState.fifoX));
  file.read(reinterpret_cast<char *>(&pipelineState.lineX), sizeof(pipelineState.lineX));
  file.read(reinterpret_cast<char *>(&pipelineState.pushedCount), sizeof(pipelineState.pushedCount));
  file.read(reinterpret_cast<char *>(&pipelineState.fetchX), sizeof(pipelineState.fetchX));
  file.read(reinterpret_cast<char *>(pipelineState.bgwBuffer.data()), pipelineState.bgwBuffer.size() * sizeof(uint8_t));
  file.read(reinterpret_cast<char *>(pipelineState.objectBuffer.data()), pipelineState.objectBuffer.size() * sizeof(uint8_t));
  file.read(reinterpret_cast<char *>(pipelineState.fetchedEntries.data()), pipelineState.fetchedEntries.size() * sizeof(OAM_ENTRY));
  file.read(reinterpret_cast<char *>(&pipelineState.mapX), sizeof(pipelineState.mapX));
  file.read(reinterpret_cast<char *>(&pipelineState.mapY), sizeof(pipelineState.mapY));
  file.read(reinterpret_cast<char *>(&pipelineState.tileX), sizeof(pipelineState.tileX));
  file.read(reinterpret_cast<char *>(&pipelineState.tileY), sizeof(pipelineState.tileY));
  file.read(reinterpret_cast<char *>(&pipelineState.entryCount), sizeof(pipelineState.entryCount));

  ppu->setState(state);
  ppu->setPipelineState(pipelineState);
}

void StateSerializer::loadLCDState(std::ifstream &file)
{
  char marker[10];
  file.read(marker, 9);
  marker[9] = '\0';

  if (strcmp(marker, "LCD_STATE") != 0)
  {
    throw std::runtime_error("Invalid LCD state marker in save file");
  }

  LCD::State state = lcd->getState();

  file.read(reinterpret_cast<char *>(&state.lcdc), sizeof(state.lcdc));
  file.read(reinterpret_cast<char *>(&state.lcds), sizeof(state.lcds));
  file.read(reinterpret_cast<char *>(&state.scrollX), sizeof(state.scrollX));
  file.read(reinterpret_cast<char *>(&state.scrollY), sizeof(state.scrollY));
  file.read(reinterpret_cast<char *>(&state.ly), sizeof(state.ly));
  file.read(reinterpret_cast<char *>(&state.lyCompare), sizeof(state.lyCompare));
  file.read(reinterpret_cast<char *>(&state.dma), sizeof(state.dma));

  file.read(reinterpret_cast<char *>(&state.bgp), sizeof(state.bgp));

  file.read(reinterpret_cast<char *>(state.palettes.data()),
            state.palettes.size() * sizeof(LCD::PaletteRegister));

  file.read(reinterpret_cast<char *>(&state.windowX), sizeof(state.windowX));
  file.read(reinterpret_cast<char *>(&state.windowY), sizeof(state.windowY));

  file.read(reinterpret_cast<char *>(state.bgColors.data()),
            state.bgColors.size() * sizeof(uint32_t));
  file.read(reinterpret_cast<char *>(state.ob1Colors.data()),
            state.ob1Colors.size() * sizeof(uint32_t));
  file.read(reinterpret_cast<char *>(state.ob2Colors.data()),
            state.ob2Colors.size() * sizeof(uint32_t));

  lcd->setState(state);
}
