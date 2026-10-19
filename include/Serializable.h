#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

class Serializable {
 public:
  virtual ~Serializable() = default;
  virtual void save(std::ostream& out) const = 0;
  virtual void load(std::istream& in) = 0;
};

namespace serial {

template <typename T>
void write(std::ostream& out, const T& value) {
  static_assert(std::is_trivially_copyable_v<T>);
  out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void read(std::istream& in, T& value) {
  static_assert(std::is_trivially_copyable_v<T>);
  in.read(reinterpret_cast<char*>(&value), sizeof(T));
  if (!in) {
    throw std::runtime_error("Unexpected end of save state data");
  }
}

inline void writeBytes(std::ostream& out, const std::vector<uint8_t>& bytes) {
  write(out, static_cast<uint64_t>(bytes.size()));
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

inline void readBytes(std::istream& in, std::vector<uint8_t>& bytes) {
  uint64_t size = 0;
  read(in, size);
  if (size != bytes.size()) {
    throw std::runtime_error("Save state buffer size does not match");
  }
  in.read(reinterpret_cast<char*>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));
  if (!in) {
    throw std::runtime_error("Unexpected end of save state data");
  }
}

}  // namespace serial
