#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "Apu.h"
#include "Bus.h"
#include "Cartridge.h"
#include "Common.h"
#include "Cpu.h"
#include "Dma.h"
#include "Gamepad.h"
#include "Io.h"
#include "Lcd.h"
#include "Logger.h"
#include "Ppu.h"
#include "Ram.h"
#include "Timer.h"

namespace {

constexpr uint64_t DEFAULT_BUDGET_MCYCLES = 300'000'000ULL;

enum class Result { Running, Pass, Fail };

enum class Protocol { Blargg, Mooneye, Memory };

constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;

template <std::size_t N>
uint64_t hashFramebuffer(const std::array<uint32_t, N> &buffer) {
  uint64_t hash = FNV_OFFSET_BASIS;
  for (uint32_t pixel : buffer) {
    for (int shift = 0; shift < 32; shift += 8) {
      hash ^= static_cast<uint8_t>(pixel >> shift);
      hash *= FNV_PRIME;
    }
  }
  return hash;
}

template <std::size_t N>
void dumpFramebuffer(const std::string &path,
                     const std::array<uint32_t, N> &buffer) {
  std::ofstream out(path, std::ios::binary);
  out << "P6\n" << XRES << ' ' << YRES << "\n255\n";
  for (uint32_t pixel : buffer) {
    const char rgb[3] = {static_cast<char>(pixel >> 16),
                         static_cast<char>(pixel >> 8),
                         static_cast<char>(pixel)};
    out.write(rgb, sizeof(rgb));
  }
}

constexpr std::string_view MOONEYE_PASS{"\x03\x05\x08\x0D\x15\x22", 6};
constexpr std::string_view MOONEYE_FAIL = "BBBBBB";
constexpr std::string_view PASS_MARKER = "Passed";
constexpr std::string_view FAIL_MARKER = "Failed";

constexpr uint16_t MEMORY_RESULT_ADDR = 0xA000;
constexpr uint16_t MEMORY_SIGNATURE_ADDR = 0xA001;
constexpr uint8_t MEMORY_SIGNATURE_0 = 0xDE;
constexpr uint8_t MEMORY_SIGNATURE_1 = 0xB0;
constexpr uint8_t MEMORY_SIGNATURE_2 = 0x61;
constexpr uint8_t MEMORY_STILL_RUNNING = 0x80;
constexpr uint8_t MEMORY_PASS_CODE = 0x00;

class TestRunner {
 public:
  TestRunner(const std::string &romPath, Protocol protocol)
      : protocol(protocol) {
    Logger::Disable();
    cartridge = std::make_unique<Cartridge>(romPath);
    dma = std::make_unique<DMA>(
        [this](uint16_t addr, uint8_t val) { ppu->oamWrite(addr, val); });
    ram = std::make_unique<RAM>();
    gamepad = std::make_unique<Gamepad>();
    lcd = std::make_unique<LCD>([this](uint8_t value) { dma->start(value); });
    apu = std::make_unique<APU>();
    cpu = std::make_unique<CPU>(&TestRunner::cycleTrampoline, this, nullptr);
    ppu = std::make_unique<PPU>(*cpu);
    timer = std::make_unique<Timer>(*cpu);
    io = std::make_unique<IO>(cpu->getInterruptRegs(), *timer, *lcd, *gamepad,
                              *apu);
    bus = std::make_unique<Bus>(*cartridge, cpu->getInterruptRegs(), *dma, *io,
                                *ppu, *ram);
    cpu->setBus(bus.get());
    dma->setMemRead(*bus);
    ppu->setMemRead(*bus);
    ppu->setLcd(lcd.get());
    io->setSerialSink([this](uint8_t byte) {
      serialLog.push_back(static_cast<char>(byte));
      serialDirty = true;
    });
    ppu->init();
  }

  int run(uint64_t budget) {
    while (totalCycles < budget) {
      cpu->step();
      switch (checkResult()) {
        case Result::Pass:
          report();
          return 0;
        case Result::Fail:
          report();
          return 1;
        case Result::Running:
          break;
      }
    }
    std::cerr << "TIMEOUT after " << budget << " cycles\n";
    report();
    return 2;
  }

  void runFrames(uint32_t frames) {
    while (framesElapsed < frames) {
      cpu->step();
    }
  }

  [[nodiscard]] const auto &videoBuffer() const {
    return ppu->getVideoBuffer();
  }

 private:
  static void cycleTrampoline(void *self, int cycles) {
    static_cast<TestRunner *>(self)->cycle(cycles);
  }

  void cycle(int cycles) {
    totalCycles += static_cast<uint64_t>(cycles);
    for (int i = 0; i < cycles; i++) {
      for (int j = 0; j < CLOCK_CYCLES; j++) {
        timer->tick();
        const uint32_t prevFrame = ppu->getCurrentFrame();
        ppu->tick();
        if (prevFrame != ppu->getCurrentFrame()) {
          frameDirty = true;
          framesElapsed++;
        }
        apu->tick();
      }
      dma->tick();
    }
  }

  Result checkResult() {
    if (protocol == Protocol::Memory) {
      if (!frameDirty) {
        return Result::Running;
      }
      frameDirty = false;
      return checkMemoryResult();
    }

    if (!serialDirty) {
      return Result::Running;
    }
    serialDirty = false;

    const std::string_view pass =
        protocol == Protocol::Mooneye ? MOONEYE_PASS : PASS_MARKER;
    const std::string_view fail =
        protocol == Protocol::Mooneye ? MOONEYE_FAIL : FAIL_MARKER;

    if (serialLog.find(pass) != std::string::npos) {
      return Result::Pass;
    }
    if (serialLog.find(fail) != std::string::npos) {
      return Result::Fail;
    }
    return Result::Running;
  }

  Result checkMemoryResult() {
    if (bus->read8(MEMORY_SIGNATURE_ADDR) != MEMORY_SIGNATURE_0 ||
        bus->read8(MEMORY_SIGNATURE_ADDR + 1) != MEMORY_SIGNATURE_1 ||
        bus->read8(MEMORY_SIGNATURE_ADDR + 2) != MEMORY_SIGNATURE_2) {
      return Result::Running;
    }

    const uint8_t code = bus->read8(MEMORY_RESULT_ADDR);
    if (code == MEMORY_STILL_RUNNING) {
      return Result::Running;
    }
    return code == MEMORY_PASS_CODE ? Result::Pass : Result::Fail;
  }

  void report() const {
    if (protocol != Protocol::Mooneye) {
      std::cout << serialLog << '\n';
      return;
    }
    std::cout << "serial:";
    for (unsigned char byte : serialLog) {
      std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte);
    }
    std::cout << std::dec << '\n';
  }

  std::unique_ptr<Cartridge> cartridge;
  std::unique_ptr<DMA> dma;
  std::unique_ptr<RAM> ram;
  std::unique_ptr<Gamepad> gamepad;
  std::unique_ptr<LCD> lcd;
  std::unique_ptr<APU> apu;
  std::unique_ptr<CPU> cpu;
  std::unique_ptr<PPU> ppu;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<IO> io;
  std::unique_ptr<Bus> bus;

  Protocol protocol = Protocol::Blargg;
  std::string serialLog;
  bool serialDirty = false;
  bool frameDirty = false;
  uint64_t totalCycles = 0;
  uint32_t framesElapsed = 0;
};

}  // namespace

int main(int argc, char **argv) {
  try {
    std::string romPath;
    uint64_t budget = DEFAULT_BUDGET_MCYCLES;
    Protocol protocol = Protocol::Blargg;
    uint32_t frames = 0;
    bool frameMode = false;
    bool printHash = false;
    std::string dumpPath;

    for (int i = 1; i < argc; i++) {
      const std::string arg = argv[i];
      if (arg == "--mooneye") {
        protocol = Protocol::Mooneye;
      } else if (arg == "--memory") {
        protocol = Protocol::Memory;
      } else if (arg == "--hash") {
        printHash = true;
      } else if (arg == "--frames") {
        frameMode = true;
        frames = static_cast<uint32_t>(std::stoul(argv[++i]));
      } else if (arg == "--dump") {
        dumpPath = argv[++i];
      } else if (romPath.empty()) {
        romPath = arg;
      } else {
        budget = std::stoull(arg) * 1'000'000ULL;
      }
    }

    if (romPath.empty()) {
      std::cerr << "Usage: test_runner <rom_path> [max_million_cycles] "
                   "[--mooneye] [--memory] [--frames N] [--hash] "
                   "[--dump path]\n";
      return 3;
    }

    TestRunner runner(romPath, protocol);

    if (frameMode) {
      runner.runFrames(frames);
      if (printHash) {
        std::cout << std::hex << std::setfill('0') << std::setw(16)
                  << hashFramebuffer(runner.videoBuffer()) << std::dec << '\n';
      }
      if (!dumpPath.empty()) {
        dumpFramebuffer(dumpPath, runner.videoBuffer());
      }
      return 0;
    }

    return runner.run(budget);
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 3;
  }
}
