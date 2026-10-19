#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

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

 private:
  std::vector<Section> sections_;

  void writeSections(std::ostream& out) const;
  void readSections(std::istream& in);

  std::string removeSpaces(const std::string& str) const;

  static constexpr std::array<char, 4> MAGIC = {'G', 'B', 'S', 'T'};
  static constexpr uint32_t VERSION = 2;
  static constexpr uint32_t MAX_HEADER_LENGTH = 256;
};
