#pragma once

#include <cstdint>
#include <functional>

class FramePacer {
 public:
  FramePacer(std::function_ref<uint32_t()> getTicksFn,
             std::function_ref<void(uint32_t)> delayFn,
             uint32_t targetFps = 60);

  void onFrameComplete();
  void setFastForward(bool ff);
  [[nodiscard]] bool isFastForward() const;

 private:
  std::function_ref<uint32_t()> getTicksFn_;
  std::function_ref<void(uint32_t)> delayFn_;
  uint32_t targetFrameTime;
  uint32_t prevFrameTime = 0;
  uint32_t startTimer = 0;
  bool fastForward = false;
};
