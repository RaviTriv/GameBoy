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

class APU;
class PPU;
class Gamepad;
class UI
{
public:
  using CloseCallback = std::function<void()>;
  using SaveStateCallback = std::function<void()>;
  UI(CloseCallback closeCallback, SaveStateCallback saveStateCallback, PPU &ppu, Gamepad &gamepad, APU &apu);
  void init();
  void handleEvents();
  void update();
  uint32_t getTicks() const;
  void delay(uint32_t ms);
  ~UI();
  APU &apu;

private:
  PPU &ppu;
  Gamepad &gamepad;
  static constexpr int SCREEN_WIDTH = 640;
  static constexpr int SCREEN_HEIGHT = 576;
  static constexpr int SCALE = 4;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Surface *screen;
  SDL_Texture *sdlTexture;
  CloseCallback onClose;
  SaveStateCallback onSaveState;

  void onKey(bool isDown, SDL_Keycode keyCode);
};

void audioCallback(void *_sound, SDL_AudioStream *_stream, int _additional_amount, int _length);
