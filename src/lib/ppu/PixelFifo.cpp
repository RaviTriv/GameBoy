#include "PixelFifo.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>

#include "Common.h"
#include "Serializable.h"

PixelFifo::PixelFifo() { buffer.fill(0); }

void PixelFifo::push(uint32_t pixel) {
  if (isFull()) {
    return;
  }

  buffer[tail] = pixel;
  tail = (tail + 1) % FIFO_CAPACITY;

  count++;
}

uint32_t PixelFifo::pop() {
  if (isEmpty()) {
    return 0;
  }

  uint32_t pixel = buffer[head];
  head = (head + 1) % FIFO_CAPACITY;

  count--;

  return pixel;
}

bool PixelFifo::isEmpty() const { return count == 0; }

bool PixelFifo::isFull() const { return count == FIFO_CAPACITY; }

size_t PixelFifo::size() const { return count; }

void PixelFifo::reset() {
  head = 0;
  tail = 0;
  count = 0;
}

void PixelFifo::save(std::ostream& out) const {
  serial::saveFields(out, *this);
}

void PixelFifo::load(std::istream& in) {
  serial::loadFields(in, *this);
  if (head >= FIFO_CAPACITY || tail >= FIFO_CAPACITY || count > FIFO_CAPACITY) {
    throw std::runtime_error("Invalid pixel FIFO state in save file");
  }
}