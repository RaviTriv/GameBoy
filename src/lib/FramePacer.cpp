#include "FramePacer.h"

FramePacer::FramePacer(std::function<uint32_t()> getTicksFn,
                       std::function<void(uint32_t)> delayFn,
                       uint32_t targetFps)
    : getTicksFn(std::move(getTicksFn)),
      delayFn(std::move(delayFn)),
      targetFrameTime(1000 / targetFps)
{
}

void FramePacer::onFrameComplete()
{
  if (fastForward)
  {
    return;
  }

  uint32_t end = getTicksFn();
  uint32_t frameTime = end - prevFrameTime;
  if (frameTime < targetFrameTime)
  {
    delayFn(targetFrameTime - frameTime);
  }
  if (end - startTimer >= 1000)
  {
    startTimer = end;
    frameCount = 0;
  }

  frameCount++;
  prevFrameTime = getTicksFn();
}

void FramePacer::setFastForward(bool ff)
{
  fastForward = ff;
}

bool FramePacer::isFastForward() const
{
  return fastForward;
}
