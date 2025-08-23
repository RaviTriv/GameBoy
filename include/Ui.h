#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_rect.h>

class UI
{
public:
  void init();
  ~UI();

private:
  static constexpr int SCREEN_WIDTH = 640;
  static constexpr int SCREEN_HEIGHT = 576;
  static constexpr int SCALE = 4;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Surface *screen;
  SDL_Texture *sdlTexture;
};