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

class APU;
class PPU;
class Gamepad;
class UI
{
public:
  using CloseCallback = std::function<void()>;
  UI(CloseCallback closeCallback, std::shared_ptr<PPU> ppu, std::shared_ptr<Gamepad> gamepad, std::shared_ptr<APU> apu);
  void setPpu(std::shared_ptr<PPU> ppu);
  void setApu(std::shared_ptr<APU> apu);
  void init();
  void handleEvents();
  void update();
  uint32_t getTicks();
  void delay(uint32_t ms);
  ~UI();
  std::shared_ptr<APU> apu;

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

void audioCallback(void *_sound, SDL_AudioStream *_stream, int _additional_amount, int _length);
