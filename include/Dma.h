#pragma once

#include <cstdint>
#include <memory>

class Bus;
class PPU;
class DMA
{
  struct State
  {
    bool isActive;
    uint8_t byte;
    uint8_t value;
    uint8_t startDelay;
  };

public:
  DMA(std::shared_ptr<Bus> bus, std::shared_ptr<PPU> ppu);
  void start(uint8_t start);
  void tick();
  bool isTransferring() const;
  void setBus(std::shared_ptr<Bus> bus) { this->bus = bus; }
  void setPpu(std::shared_ptr<PPU> ppu) { this->ppu = ppu; }
private:
  State state;
  std::shared_ptr<Bus> bus;
  std::shared_ptr<PPU> ppu;
};