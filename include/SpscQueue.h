#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <span>

template <typename T, std::size_t Capacity>
class SPSCQueue {
  static_assert(std::has_single_bit(Capacity), "Capacity must be a power of 2");

 public:
  bool push(const T& item) {
    const std::size_t t = tail.load(std::memory_order_relaxed);
    const std::size_t next = (t + 1) & MASK;
    if (next == cachedHead) {
      cachedHead = head.load(std::memory_order_acquire);
      if (next == cachedHead) {
        return false;
      }
    }
    buffer[t] = item;
    tail.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& item) { return pop(std::span<T>{&item, 1}) == 1; }

  std::size_t pop(std::span<T> out) {
    const std::size_t h = head.load(std::memory_order_relaxed);
    std::size_t available = (cachedTail - h) & MASK;
    if (available < out.size()) {
      cachedTail = tail.load(std::memory_order_acquire);
      available = (cachedTail - h) & MASK;
    }
    const std::size_t count = std::min(out.size(), available);
    if (count == 0) {
      return 0;
    }
    const std::size_t firstChunk = std::min(count, Capacity - h);
    const std::span<const T, Capacity> ring{buffer};
    std::ranges::copy(ring.subspan(h, firstChunk), out.begin());
    std::ranges::copy(ring.first(count - firstChunk),
                      out.subspan(firstChunk).begin());
    head.store((h + count) & MASK, std::memory_order_release);
    return count;
  }

 private:
  static constexpr std::size_t MASK = Capacity - 1;
  static constexpr std::size_t CACHE_LINE = 64;

  alignas(CACHE_LINE) std::atomic<std::size_t> head{0};
  std::size_t cachedTail = 0;
  alignas(CACHE_LINE) std::atomic<std::size_t> tail{0};
  std::size_t cachedHead = 0;
  alignas(CACHE_LINE) std::array<T, Capacity> buffer{};
};
