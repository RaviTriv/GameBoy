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

void PixelFifo::save(std::ostream &out) const {
  serial::write(out, buffer);
  serial::write(out, head);
  serial::write(out, tail);
  serial::write(out, count);
}

void PixelFifo::load(std::istream &in) {
  std::array<uint32_t, FIFO_CAPACITY> newBuffer{};
  size_t newHead = 0;
  size_t newTail = 0;
  size_t newCount = 0;
  serial::read(in, newBuffer);
  serial::read(in, newHead);
  serial::read(in, newTail);
  serial::read(in, newCount);
  if (newHead >= FIFO_CAPACITY || newTail >= FIFO_CAPACITY ||
      newCount > FIFO_CAPACITY) {
    throw std::runtime_error("Invalid pixel FIFO state in save file");
  }
  buffer = newBuffer;
  head = newHead;
  tail = newTail;
  count = newCount;
}