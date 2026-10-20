#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

class Cartridge;
class Serializable;

class StateSerializer {
 public:
  struct Section {
    std::string name;
    Serializable* component;
  };

  explicit StateSerializer(std::vector<Section> sections);
  [[nodiscard]] bool save(std::ostream& out, const std::string& label = {});
  [[nodiscard]] bool load(std::istream& in);
  [[nodiscard]] bool saveState(const std::string& title);
  [[nodiscard]] bool loadState(const std::string& title);
  static bool saveBattery(const Cartridge& cartridge);
  static bool loadBattery(Cartridge& cartridge);

 private:
  std::vector<Section> sections_;

  void writeSections(std::ostream& out) const;
  void readSections(std::istream& in);

  static std::string removeSpaces(const std::string& str);
  static std::filesystem::path savePath(const std::string& title,
                                        const std::string& extension);

  static constexpr std::array<char, 4> MAGIC = {'G', 'B', 'S', 'T'};
  static constexpr uint32_t VERSION = 2;
  static constexpr uint32_t MAX_HEADER_LENGTH = 256;
};
