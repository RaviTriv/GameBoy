#include "StateSerializer.h"

#include <array>
#include <cstdint>
#include <ctime>
#include <exception>
#include <ios>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Logger.h"
#include "Serializable.h"

namespace {

std::streamsize streamSize(std::size_t bytes) {
  return static_cast<std::streamsize>(bytes);
}

std::string readString(std::istream& in, std::size_t length) {
  std::string value(length, '\0');
  in.read(value.data(), streamSize(length));
  if (!in) {
    throw std::runtime_error("Unexpected end of save state data");
  }
  return value;
}

}  // namespace

StateSerializer::StateSerializer(std::vector<Section> sections)
    : sections_(std::move(sections)) {}

bool StateSerializer::save(std::ostream& out, const std::string& label) {
  try {
    std::stringstream headerStream;
    headerStream << label << "_" << std::time(nullptr);
    std::string header = headerStream.str();

    out.write(MAGIC.data(), streamSize(MAGIC.size()));
    serial::write(out, VERSION);
    serial::write(out, static_cast<uint32_t>(header.size()));
    out.write(header.data(), streamSize(header.size()));
    writeSections(out);
    return out.good();
  } catch (const std::exception& e) {
    Logger::GetLogger()->error("Error saving state: {}", e.what());
    return false;
  }
}

bool StateSerializer::load(std::istream& in) {
  try {
    if (readString(in, MAGIC.size()) !=
        std::string(MAGIC.data(), MAGIC.size())) {
      throw std::runtime_error("Not a save state file");
    }
    uint32_t version = 0;
    serial::read(in, version);
    if (version != VERSION) {
      throw std::runtime_error("Unsupported save state version " +
                               std::to_string(version));
    }
    uint32_t headerLength = 0;
    serial::read(in, headerLength);
    if (headerLength > MAX_HEADER_LENGTH) {
      throw std::runtime_error("Invalid save state header");
    }
    readString(in, headerLength);
  } catch (const std::exception& e) {
    Logger::GetLogger()->error("Error loading state: {}", e.what());
    return false;
  }

  std::stringstream backup;
  writeSections(backup);

  try {
    readSections(in);
    return true;
  } catch (const std::exception& e) {
    Logger::GetLogger()->error("Error loading state: {}", e.what());
    backup.seekg(0);
    readSections(backup);
    return false;
  }
}

void StateSerializer::writeSections(std::ostream& out) const {
  serial::write(out, static_cast<uint32_t>(sections_.size()));
  for (const auto& section : sections_) {
    std::ostringstream payload;
    section.component->save(payload);
    const std::string bytes = payload.str();

    serial::write(out, static_cast<uint8_t>(section.name.size()));
    out.write(section.name.data(), streamSize(section.name.size()));
    serial::write(out, static_cast<uint32_t>(bytes.size()));
    out.write(bytes.data(), streamSize(bytes.size()));
  }
}

void StateSerializer::readSections(std::istream& in) {
  uint32_t count = 0;
  serial::read(in, count);
  if (count != sections_.size()) {
    throw std::runtime_error("Save state has " + std::to_string(count) +
                             " sections, expected " +
                             std::to_string(sections_.size()));
  }

  for (const auto& section : sections_) {
    uint8_t nameLength = 0;
    serial::read(in, nameLength);
    const std::string name = readString(in, nameLength);
    if (name != section.name) {
      throw std::runtime_error("Expected section " + section.name + ", found " +
                               name);
    }

    uint32_t size = 0;
    serial::read(in, size);
    std::istringstream payload(readString(in, size));
    section.component->load(payload);
    if (payload.peek() != std::istringstream::traits_type::eof()) {
      throw std::runtime_error("Section " + section.name +
                               " has unexpected trailing data");
    }
  }
}
