#pragma once

#include "./Common.h"

#include <array>
#include <cstdint>

class PixelFifo
{

private:
  std::array<uint32_t, FIFO_CAPACITY> buffer;
  size_t head = 0;
  size_t tail = 0;
  size_t count = 0;

public:
  PixelFifo();
  ~PixelFifo() = default;

  void push(uint32_t pixel);
  uint32_t pop();

  bool isEmpty() const;
  bool isFull() const;
  size_t size() const;

  void reset();

  const std::array<uint32_t, FIFO_CAPACITY> &getBuffer() const;
  size_t getHead() const;
  size_t getTail() const;
  size_t getCount() const;
  void setState(const std::array<uint32_t, FIFO_CAPACITY> &newBuffer,
                size_t newHead, size_t newTail, size_t newCount);
};