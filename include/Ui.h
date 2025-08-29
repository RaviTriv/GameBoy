#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_rect.h>
#include <functional>
#include <memory>

class PPU;
class Gamepad;
class UI
{
public:
  using CloseCallback = std::function<void()>;
  UI(CloseCallback closeCallback, std::shared_ptr<PPU> ppu, std::shared_ptr<Gamepad> gamepad);
  void setPpu(std::shared_ptr<PPU> ppu);
  void init();
  void handleEvents();
  void update();
  uint32_t getTicks();
  void delay(uint32_t ms);
  ~UI();

private:
  std::shared_ptr<PPU> ppu;
  std::shared_ptr<Gamepad> gamepad;

  static constexpr int YRES = 144;
  static constexpr int XRES = 160;
  static constexpr int SCREEN_WIDTH = 640;
  static constexpr int SCREEN_HEIGHT = 576;
  static constexpr int SCALE = 4;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Surface *screen;
  SDL_Texture *sdlTexture;
  CloseCallback onClose;

  void onKey(bool isDown, SDL_Keycode keyCode);
};