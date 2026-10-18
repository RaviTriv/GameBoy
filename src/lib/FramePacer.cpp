#include "FramePacer.h"

#include <cstdint>
#include <functional>
#include <utility>

FramePacer::FramePacer(std::function<uint32_t()> getTicksFn,
                       std::function<void(uint32_t)> delayFn,
                       uint32_t targetFps)
    : getTicksFn_(std::move(getTicksFn)),
      delayFn_(std::move(delayFn)),
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
