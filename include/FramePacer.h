#pragma once

#include <cstdint>
#include <functional>

class FramePacer
{
public:
  FramePacer(std::function<uint32_t()> getTicksFn,
             std::function<void(uint32_t)> delayFn,
             uint32_t targetFps = 60);

  void onFrameComplete();
  void setFastForward(bool ff);
  [[nodiscard]] bool isFastForward() const;

private:
  std::function<uint32_t()> getTicksFn;
  std::function<void(uint32_t)> delayFn;
  uint32_t targetFrameTime;
  long prevFrameTime = 0;
  long startTimer = 0;
  long frameCount = 0;
  bool fastForward = false;
};
