#pragma once

class PPU;
class Pipeline
{
public:
  Pipeline(PPU *ppu);
  void process();
  void reset();

private:
  PPU *ppu;
};