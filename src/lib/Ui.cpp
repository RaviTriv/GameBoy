#include "../../include/Ui.h"
#include "../../include/Apu.h"
#include "../../include/Ppu.h"
#include "../../include/Gamepad.h"
#include "../../include/Logger.h"

UI::UI(CloseCallback closeCallback, std::shared_ptr<PPU> ppu, std::shared_ptr<Gamepad> gamepad, std::shared_ptr<APU> apu) : onClose(closeCallback), ppu(ppu), gamepad(gamepad), apu(apu)
{
}

void UI::setPpu(std::shared_ptr<PPU> ppu) { this->ppu = ppu; }
void UI::setApu(std::shared_ptr<APU> apu) { this->apu = apu; }

void UI::init()
{
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  SDL_CreateWindowAndRenderer("GAMEBOY EMULATOR", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
  screen = SDL_CreateSurface(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_ARGB8888);
  sdlTexture = SDL_CreateTexture(renderer,
                                 SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 SCREEN_WIDTH, SCREEN_HEIGHT);

  SDL_InitSubSystem(SDL_INIT_AUDIO);
  const SDL_AudioSpec spec = {SDL_AUDIO_U8, 2, apu->audioFreq};
  SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioCallback, this);
  SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(stream));
}

void audioCallback(void *_sound, SDL_AudioStream *_stream, int _additional_amount, int _length)
{
  uint8_t *stream = (uint8_t *)_stream;
  UI *ui = (UI *)_sound;
  int length = _length / sizeof(stream[0]);

  uint8_t *data = SDL_stack_alloc(uint8_t, _length);
  for (int i = 0; i < _length; i += 2)
  {
    uint8_t sample = ui->apu->getSample();
    data[i] = sample;
    data[i + 1] = sample;
  }
  SDL_PutAudioStreamData(_stream, data, _length);
  SDL_stack_free(data);
}

void UI::update()
{
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = SCALE;
  rect.h = SCALE;

  const auto &videoBuffer = ppu->getVideoBuffer();

  for (int i = 0; i < YRES; i++)
  {
    for (int j = 0; j < XRES; j++)
    {
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

void UI::onKey(bool isDown, SDL_Keycode keyCode)
{
  switch (keyCode)
  {
  case SDLK_Z:
    gamepad->setBPressed(isDown);
    break;
  case SDLK_X:
    gamepad->setAPressed(isDown);
    break;
  case SDLK_RETURN:
    gamepad->setStartPressed(isDown);
    break;
  case SDLK_TAB:
    gamepad->setSelectPressed(isDown);
    break;
  case SDLK_UP:
    gamepad->setUpPressed(isDown);
    break;
  case SDLK_DOWN:
    gamepad->setDownPressed(isDown);
    break;
  case SDLK_LEFT:
    gamepad->setLeftPressed(isDown);
    break;
  case SDLK_RIGHT:
    gamepad->setRightPressed(isDown);
    break;
  case SDLK_S:
    // TODO: Save State
    break;
  }
}

void UI::handleEvents()
{
  SDL_Event e;

  while (SDL_PollEvent(&e) > 0)
  {
    switch (e.type)
    {
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

void UI::delay(uint32_t ms)
{
  SDL_Delay(ms);
}

uint32_t UI::getTicks()
{
  return SDL_GetTicks();
}

UI::~UI()
{
  SDL_DestroyTexture(sdlTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
}
