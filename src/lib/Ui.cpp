#include "Ui.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "Apu.h"
#include "Common.h"
#include "Gamepad.h"
#include "Logger.h"
#include "Ppu.h"

UI::UI(CloseCallback closeCallback, SaveStateCallback saveStateCallback,
       PPU &ppu, Gamepad &gamepad, APU &apu)
    : apu(apu),
      ppu(ppu),
      gamepad(gamepad),
      onClose(closeCallback),
      onSaveState(saveStateCallback) {}

void UI::init() {
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  SDL_CreateWindowAndRenderer("GAMEBOY EMULATOR", SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_RESIZABLE, &window, &renderer);
  screen =
      SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_ARGB8888);
  sdlTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                                 SCREEN_HEIGHT);

  SDL_InitSubSystem(SDL_INIT_AUDIO);
  const SDL_AudioSpec spec = {SDL_AUDIO_U8, 2, apu.audioFreq};
  SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(
      SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioCallback, this);
  SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
}

namespace {

constexpr int BYTES_PER_FRAME = 2;
static_assert(sizeof(StereoSample) == BYTES_PER_FRAME,
              "StereoSample must be a packed U8 stereo frame");

constexpr std::size_t MAX_CHUNK_FRAMES = 2048;

}  // namespace

void audioCallback(void *sound, SDL_AudioStream *stream, int additionalAmount,
                   int totalAmount) {
  (void)totalAmount;

  if (additionalAmount <= 0) {
    return;
  }

  UI *ui = static_cast<UI *>(sound);

  std::array<StereoSample, MAX_CHUNK_FRAMES> chunk;

  int remaining = additionalAmount;
  while (remaining >= BYTES_PER_FRAME) {
    const std::size_t frames = std::min<std::size_t>(
        static_cast<std::size_t>(remaining / BYTES_PER_FRAME),
        MAX_CHUNK_FRAMES);

    const std::size_t popped = ui->apu.popSamples(chunk.data(), frames);
    for (std::size_t i = popped; i < frames; i++) {
      chunk[i] = {};
    }

    const int bytes = static_cast<int>(frames * BYTES_PER_FRAME);
    SDL_PutAudioStreamData(stream, chunk.data(), bytes);
    remaining -= bytes;
  }
}

void UI::update() {
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = SCALE;
  rect.h = SCALE;

  const auto &videoBuffer = ppu.getVideoBuffer();

  for (int i = 0; i < YRES; i++) {
    for (int j = 0; j < XRES; j++) {
      rect.x = j * SCALE;
      rect.y = i * SCALE;
      rect.w = SCALE;
      rect.h = SCALE;

      SDL_FillSurfaceRect(screen, &rect, videoBuffer[j + (i * XRES)]);
    }
  }

  SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, sdlTexture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void UI::onKey(bool isDown, SDL_Keycode keyCode) {
  switch (keyCode) {
    case SDLK_Z:
      gamepad.setBPressed(isDown);
      break;
    case SDLK_X:
      gamepad.setAPressed(isDown);
      break;
    case SDLK_RETURN:
      gamepad.setStartPressed(isDown);
      break;
    case SDLK_TAB:
      gamepad.setSelectPressed(isDown);
      break;
    case SDLK_UP:
      gamepad.setUpPressed(isDown);
      break;
    case SDLK_DOWN:
      gamepad.setDownPressed(isDown);
      break;
    case SDLK_LEFT:
      gamepad.setLeftPressed(isDown);
      break;
    case SDLK_RIGHT:
      gamepad.setRightPressed(isDown);
      break;
    case SDLK_S:
      if (isDown) {
        onSaveState();
      }
      break;
    case SDLK_F:
      if (isDown && onToggleFastForward) {
        onToggleFastForward();
      }
      break;
  }
}

void UI::handleEvents() {
  SDL_Event e;

  while (SDL_PollEvent(&e) > 0) {
    switch (e.type) {
      case SDL_EVENT_KEY_DOWN:
        onKey(true, e.key.key);
        break;
      case SDL_EVENT_KEY_UP:
        onKey(false, e.key.key);
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        onClose();
      default:
        break;
    }
  }
}

void UI::delay(uint32_t ms) { SDL_Delay(ms); }

void UI::setToggleFastForward(ToggleFastForwardCallback fn) {
  onToggleFastForward = std::move(fn);
}

uint32_t UI::getTicks() const { return SDL_GetTicks(); }

UI::~UI() {
  SDL_DestroyTexture(sdlTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
}
