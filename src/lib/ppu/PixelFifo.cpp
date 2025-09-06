#include "../../../include/PixelFifo.h"

PixelFifo::PixelFifo() : head(0), tail(0), count(0)
{
  buffer.fill(0);
}

void PixelFifo::push(uint32_t pixel)
{
  if (isFull())
  {
    return;
  }

  buffer[tail] = pixel;
  tail = (tail + 1) % FIFO_CAPACITY;

  count++;
}

uint32_t PixelFifo::pop()
{
  if (isEmpty())
  {
    return 0;
  }

  uint32_t pixel = buffer[head];
  head = (head + 1) % FIFO_CAPACITY;

  count--;

  return pixel;
}

bool PixelFifo::isEmpty() const
{
  return count == 0;
}

bool PixelFifo::isFull() const
{
  return count == FIFO_CAPACITY;
}

size_t PixelFifo::size() const
{
  return count;
}

void PixelFifo::reset()
{
  head = 0;
  tail = 0;
  count = 0;
}

const std::array<uint32_t, FIFO_CAPACITY> &PixelFifo::getBuffer() const
{
  return buffer;
}
size_t PixelFifo::getHead() const { return head; }
size_t PixelFifo::getTail() const { return tail; }
size_t PixelFifo::getCount() const { return count; }

void PixelFifo::setState(const std::array<uint32_t, FIFO_CAPACITY> &newBuffer,
                         size_t newHead, size_t newTail, size_t newCount)
{
  buffer = newBuffer;
  head = newHead;
  tail = newTail;
  count = newCount;
}