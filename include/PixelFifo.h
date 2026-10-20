#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

#include "./Common.h"

class PixelFifo {
 private:
  std::array<uint32_t, FIFO_CAPACITY> buffer{};
  size_t head = 0;
  size_t tail = 0;
  size_t count = 0;

 public:
  PixelFifo();

  void push(uint32_t pixel);
  [[nodiscard]] uint32_t pop();

  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] bool isFull() const;
  [[nodiscard]] size_t size() const;

  void reset();

  void save(std::ostream& out) const;
  void load(std::istream& in);
};