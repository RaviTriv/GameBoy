#include "../../include/Ui.h"
#include "../../include/Logger.h"

UI::UI(CloseCallback closeCallback) : onClose(closeCallback)
{
}

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
