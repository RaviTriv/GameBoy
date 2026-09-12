#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "Logger.h"
#include "StateSerializer.h"

std::string StateSerializer::removeSpaces(const std::string &str) const {
  std::string result = str;
  result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
  return result;
}

bool StateSerializer::saveState(const std::string &title) {
  std::string saveTitle = removeSpaces(title);
  std::filesystem::path savesDir = "../saves";

  std::error_code ec;
  if (!std::filesystem::exists(savesDir, ec)) {
    std::filesystem::create_directory(savesDir, ec);
    if (ec) {
      Logger::GetLogger()->error("Failed to create saves directory: {}",
                                 ec.message());
      return false;
    }
  }

  std::filesystem::path saveFile = savesDir / (saveTitle + ".sav");
  std::string fileName = saveFile.string();
  std::ofstream file(fileName, std::ios::binary);

  if (!file.is_open()) {
    Logger::GetLogger()->error("Failed to create save state file: {}",
                               fileName);
    return false;
  }

  if (!save(file, saveTitle)) {
    return false;
  }

  Logger::GetLogger()->info("State successfully saved to: {}", fileName);
  return true;
}

bool StateSerializer::loadState(const std::string &title) {
  std::string saveTitle = removeSpaces(title);
  std::filesystem::path savesDir = "../saves";
  std::filesystem::path saveFile = savesDir / (saveTitle + ".sav");
  std::string fileName = saveFile.string();

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
