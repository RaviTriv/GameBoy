#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <system_error>

#include "Cartridge.h"
#include "Logger.h"
#include "StateSerializer.h"

std::string StateSerializer::removeSpaces(const std::string& str) {
  std::string result = str;
  result.erase(std::ranges::remove(result, ' ').begin(), result.end());
  return result;
}

std::filesystem::path StateSerializer::savePath(const std::string& title,
                                                const std::string& extension) {
  return std::filesystem::path("../saves") / (removeSpaces(title) + extension);
}

bool StateSerializer::saveState(const std::string& title) {
  std::filesystem::path saveFile = savePath(title, ".sav");
  std::filesystem::path savesDir = saveFile.parent_path();

  std::error_code ec;
  if (!std::filesystem::exists(savesDir, ec)) {
    std::filesystem::create_directory(savesDir, ec);
    if (ec) {
      Logger::GetLogger()->error("Failed to create saves directory: {}",
                                 ec.message());
      return false;
    }
  }

  std::string fileName = saveFile.string();
  std::ofstream file(fileName, std::ios::binary);

  if (!file.is_open()) {
    Logger::GetLogger()->error("Failed to create save state file: {}",
                               fileName);
    return false;
  }

  if (!save(file, removeSpaces(title))) {
    return false;
  }

  Logger::GetLogger()->info("State successfully saved to: {}", fileName);
  return true;
}

bool StateSerializer::loadState(const std::string& title) {
  std::string fileName = savePath(title, ".sav").string();

  std::ifstream file(fileName, std::ios::binary);
  if (!file.is_open()) {
    Logger::GetLogger()->error("Failed to open save state file: {}", fileName);
    return false;
  }

  if (!load(file)) {
    return false;
  }

  Logger::GetLogger()->info("State successfully loaded from: {}", fileName);
  return true;
}

bool StateSerializer::saveBattery(const Cartridge& cartridge) {
  if (!cartridge.hasBattery()) {
    return true;
  }
  const std::filesystem::path path = savePath(cartridge.getTitle(), ".ram");
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  std::filesystem::path tmpPath = path;
  tmpPath += ".tmp";
  {
    std::ofstream file(tmpPath, std::ios::binary | std::ios::trunc);
    cartridge.saveBattery(file);
    if (!file.good()) {
      Logger::GetLogger()->error("Failed to write battery save: {}",
                                 tmpPath.string());
      return false;
    }
  }
  std::filesystem::rename(tmpPath, path, ec);
  if (ec) {
    Logger::GetLogger()->error("Failed to write battery save {}: {}",
                               path.string(), ec.message());
    return false;
  }
  Logger::GetLogger()->info("Battery save written to: {}", path.string());
  return true;
}

bool StateSerializer::loadBattery(Cartridge& cartridge) {
  if (!cartridge.hasBattery()) {
    return true;
  }
  const std::filesystem::path path = savePath(cartridge.getTitle(), ".ram");
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  cartridge.loadBattery(file);
  Logger::GetLogger()->info("Battery save loaded from: {}", path.string());
  return true;
}
