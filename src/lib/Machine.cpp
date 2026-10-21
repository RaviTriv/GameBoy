#include "Machine.h"

#include <cctype>
#include <cstdint>
#include <istream>
#include <meta>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "Common.h"
#include "Serializable.h"
#include "StateSerializer.h"

Machine::Machine(std::string_view romPath)
    : cartridge_(romPath),
      lcd_(dma_),
      ppu_(interrupts_, lcd_),
      timer_(interrupts_),
      io_(interrupts_, timer_, lcd_, gamepad_, apu_),
      bus_(cartridge_, interrupts_, dma_, io_, ppu_, ram_),
      cpu_(interrupts_, bus_, &Machine::cycleCallback, this),
      serializer_(saveSections()) {}

namespace {

std::string sectionName(std::string_view member) {
  if (member.ends_with('_')) {
    member.remove_suffix(1);
  }
  std::string name;
  for (char c : member) {
    name += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return name;
}

template <typename Owner>
std::vector<StateSerializer::Section> sectionsOf(Owner& owner) {
  std::vector<StateSerializer::Section> sections;
  constexpr auto ctx = std::meta::access_context::unchecked();
  template for (constexpr auto member : std::define_static_array(
                    std::meta::nonstatic_data_members_of(^^Owner, ctx))) {
    if constexpr (!serial::isSkipped(member)) {
      using Component = typename[:std::meta::type_of(member):];
      static_assert(serial::SelfSerializing<Component>,
                    "Machine members must have save/load or be annotated "
                    "[[=serial::skip]]");
      Component& component = owner.[:member:];
      sections.push_back(
          {.name = sectionName(std::meta::identifier_of(member)),
           .save = [&component](std::ostream& out) { component.save(out); },
           .load = [&component](std::istream& in) { component.load(in); }});
    }
  }
  return sections;
}

}  // namespace

std::vector<StateSerializer::Section> Machine::saveSections() {
  return sectionsOf(*this);
}

void Machine::cycleCallback(void* self, int cycles) {
  static_cast<Machine*>(self)->cycle(cycles);
}

void Machine::cycle(int cycles) {
  cycles_ += static_cast<uint64_t>(cycles);
  cartridge_.tick(cycles);
  for (int i = 0; i < cycles; i++) {
    for (int j = 0; j < CLOCK_CYCLES; j++) {
      timer_.tick();
      ppu_.tick();
      apu_.tick();
    }
    dma_.tick(bus_, ppu_);
  }
}
