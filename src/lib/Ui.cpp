#include "../../include/Ui.h"
#include "../../include/Ppu.h"
#include "../../include/Logger.h"

UI::UI(CloseCallback closeCallback, std::shared_ptr<PPU> ppu) : onClose(closeCallback), ppu(ppu)
{
}

void UI::setPpu(std::shared_ptr<PPU> ppu) { this->ppu = ppu; }

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

void UI::handleEvents()
{
  SDL_Event e;

  while (SDL_PollEvent(&e) > 0)
  {
    switch (e.type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      onClose();
    default:
      break;
    }
  }
}

UI::~UI()
{
  SDL_DestroyTexture(sdlTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();
}
