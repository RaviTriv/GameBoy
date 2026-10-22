#include "FramePacer.h"

#include <cstdint>
#include <functional>

FramePacer::FramePacer(std::function_ref<uint32_t()> getTicksFn,
                       std::function_ref<void(uint32_t)> delayFn,
                       uint32_t targetFps)
    : getTicksFn_(getTicksFn),
      delayFn_(delayFn),
      targetFrameTime(1000 / targetFps) {}

void FramePacer::onFrameComplete() {
  if (fastForward) {
    return;
  }

  uint32_t end = getTicksFn_();
  uint32_t frameTime = end - prevFrameTime;
  if (frameTime < targetFrameTime) {
    delayFn_(targetFrameTime - frameTime);
  }
  if (end - startTimer >= 1000) {
    startTimer = end;
  }

  prevFrameTime = getTicksFn_();
}

void FramePacer::setFastForward(bool ff) { fastForward = ff; }

bool FramePacer::isFastForward() const { return fastForward; }
