// clang-format Language: Cpp
#pragma once

#include <cstdint>
#include <istream>
#include <ostream>

#include "Serializable.h"

enum class InterruptType : uint8_t {
  VBLANK = 1,
  LCD_STAT = 2,
  TIMER = 4,
  SERIAL = 8,
  JOYPAD = 16
};

class Interrupts {
 public:
  struct State {
    uint8_t ie = 0;
    uint8_t intf = 0x01;
  };

  [[nodiscard]] uint8_t ie() const { return state.ie; }
  void setIe(uint8_t value) { state.ie = value; }
  [[nodiscard]] uint8_t intf() const { return state.intf; }
  void setIntf(uint8_t value) { state.intf = value; }

  void request(InterruptType type) { state.intf |= static_cast<uint8_t>(type); }
  void requestLate(InterruptType type) {
    lateIntf_ |= static_cast<uint8_t>(static_cast<uint8_t>(type) & ~state.intf);
    request(type);
  }
  void clearLate() { lateIntf_ = 0; }
  void clear(InterruptType type) {
    state.intf &= static_cast<uint8_t>(~static_cast<uint8_t>(type));
  }
  [[nodiscard]] bool isPending(InterruptType type) const {
    return (state.intf & state.ie & static_cast<uint8_t>(type)) != 0;
  }
  [[nodiscard]] bool anyPending() const {
    return (state.intf & state.ie & 0x1F) != 0;
  }
  [[nodiscard]] bool anyPendingEarly() const {
    return (state.intf & ~lateIntf_ & state.ie & 0x1F) != 0;
  }

  void save(std::ostream& out) const { serial::saveFields(out, *this); }
  void load(std::istream& in) { serial::loadFields(in, *this); }

 private:
  State state;
  [[= serial::skip]] uint8_t lateIntf_ = 0;
};
