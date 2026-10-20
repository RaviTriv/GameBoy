#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
#include "StateSerializer.h"
#include "Timer.h"

namespace {

constexpr uint64_t DEFAULT_BUDGET_MCYCLES = 300'000'000ULL;

constexpr uint64_t MCYCLES_PER_FRAME = 17'556;
constexpr uint32_t DEFAULT_BENCH_FRAMES = 3'600;
constexpr uint32_t DEFAULT_BENCH_RUNS = 10;
constexpr uint32_t DEFAULT_BENCH_WARMUP = 2;
constexpr uint32_t MAX_BENCH_RUNS = 10'000;
constexpr double MAX_RELATIVE_STDDEV = 0.03;
constexpr std::size_t AUDIO_DRAIN_CHUNK = 1024;
constexpr uint32_t SAVESTATE_WARMUP_FRAMES = 10;
constexpr uint32_t SAVESTATE_COMPARE_FRAMES = 120;
constexpr uint32_t SAVESTATE_DRAWING_STEPS = 4;
constexpr uint8_t VISIBLE_LINES = 144;
constexpr uint32_t INPUT_HOLD_FRAMES = 5;

#ifdef __OPTIMIZE__
constexpr bool OPTIMIZED_BUILD = true;
#else
constexpr bool OPTIMIZED_BUILD = false;
#endif

#ifdef NDEBUG
constexpr bool ASSERTS_DISABLED = true;
#else
constexpr bool ASSERTS_DISABLED = false;
#endif

enum class Result { Running, Pass, Fail };

enum class Protocol { Blargg, Mooneye, Memory };

struct InputEvent {
  uint32_t frame = 0;
  char button = 0;
};

std::vector<InputEvent> parseInput(const std::string& spec) {
  std::vector<InputEvent> events;
  std::stringstream stream(spec);
  std::string item;
  while (std::getline(stream, item, ',')) {
    const auto colon = item.find(':');
    if (colon == std::string::npos || colon + 2 != item.size()) {
      throw std::invalid_argument("--input expects FRAME:BUTTON[,...]");
    }
    events.push_back(
        {.frame = static_cast<uint32_t>(std::stoul(item.substr(0, colon))),
         .button = item[colon + 1]});
  }
  return events;
}

constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;

template <std::size_t N>
uint64_t hashFramebuffer(const std::array<uint32_t, N>& buffer) {
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
void dumpFramebuffer(const std::string& path,
                     const std::array<uint32_t, N>& buffer) {
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
  TestRunner(const std::string& romPath, Protocol protocol)
      : protocol_(protocol) {
    Logger::Disable();
    cartridge = std::make_unique<Cartridge>(romPath);
    dma = std::make_unique<DMA>(
        [this](uint16_t addr, uint8_t val) { ppu->oamWrite(addr, val); });
    ram = std::make_unique<RAM>();
    gamepad = std::make_unique<Gamepad>();
    lcd = std::make_unique<LCD>([this](uint8_t value) { dma->start(value); });
    apu = std::make_unique<APU>();
    cpu = std::make_unique<CPU>(&TestRunner::cycleTrampoline, this, nullptr);
    ppu = std::make_unique<PPU>(cpu->getInterruptRegs());
    timer = std::make_unique<Timer>(cpu->getInterruptRegs());
    io = std::make_unique<IO>(cpu->getInterruptRegs(), *timer, *lcd, *gamepad,
                              *apu);
    bus = std::make_unique<Bus>(*cartridge, cpu->getInterruptRegs(), *dma, *io,
                                *ppu, *ram);
    cpu->setMemory(bus.get());
    dma->setMemRead(*bus);
    ppu->setLcd(lcd.get());
    io->setSerialSink([this](uint8_t byte) {
      serialLog.push_back(static_cast<char>(byte));
      serialDirty = true;
    });
    ppu->init();
    ppu->setOnFrameComplete([this] {
      frameDirty = true;
      framesElapsed++;
      drainAudio();
      applyInput();
    });
    stateSerializer =
        std::make_unique<StateSerializer>(std::vector<StateSerializer::Section>{
            {.name = "CPU", .component = cpu.get()},
            {.name = "RAM", .component = ram.get()},
            {.name = "PPU", .component = ppu.get()},
            {.name = "LCD", .component = lcd.get()},
            {.name = "TIMER", .component = timer.get()},
            {.name = "DMA", .component = dma.get()},
            {.name = "APU", .component = apu.get()},
            {.name = "CARTRIDGE", .component = cartridge.get()},
            {.name = "JOYPAD", .component = gamepad.get()}});
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

  [[nodiscard]] bool runUntilDrawing(uint8_t line, uint32_t extraSteps,
                                     uint32_t maxFrames) {
    while (lcd->getLy() != line || lcd->getLcdMode() != LCD::DRAWING) {
      if (framesElapsed >= maxFrames) {
        return false;
      }
      cpu->step();
    }
    for (uint32_t i = 0; i < extraSteps && lcd->getLcdMode() == LCD::DRAWING;
         i++) {
      cpu->step();
    }
    return lcd->getLy() == line && lcd->getLcdMode() == LCD::DRAWING;
  }

  void runCycles(uint64_t mcycles) {
    while (totalCycles < mcycles) {
      cpu->step();
    }
  }

  [[nodiscard]] const auto& videoBuffer() const {
    return ppu->getPublishedFrame();
  }

  [[nodiscard]] StateSerializer& serializer() { return *stateSerializer; }

  void setInput(std::vector<InputEvent> events) { input = std::move(events); }

 private:
  static void cycleTrampoline(void* self, int cycles) {
    static_cast<TestRunner*>(self)->cycle(cycles);
  }

  void cycle(int cycles) {
    totalCycles += static_cast<uint64_t>(cycles);
    cartridge->tick(cycles);
    for (int i = 0; i < cycles; i++) {
      for (int j = 0; j < CLOCK_CYCLES; j++) {
        timer->tick();
        ppu->tick();
        apu->tick();
      }
      dma->tick();
    }
  }

  void applyInput() {
    for (const InputEvent& event : input) {
      if (framesElapsed == event.frame) {
        setButton(event.button, true);
      } else if (framesElapsed == event.frame + INPUT_HOLD_FRAMES) {
        setButton(event.button, false);
      }
    }
  }

  void setButton(char button, bool pressed) {
    switch (button) {
      case 'A':
        gamepad->setAPressed(pressed);
        break;
      case 'B':
        gamepad->setBPressed(pressed);
        break;
      case 'S':
        gamepad->setStartPressed(pressed);
        break;
      case 'E':
        gamepad->setSelectPressed(pressed);
        break;
      case 'U':
        gamepad->setUpPressed(pressed);
        break;
      case 'D':
        gamepad->setDownPressed(pressed);
        break;
      case 'L':
        gamepad->setLeftPressed(pressed);
        break;
      case 'R':
        gamepad->setRightPressed(pressed);
        break;
      default:
        throw std::invalid_argument(std::string("Unknown button: ") + button);
    }
  }

  void drainAudio() {
    std::size_t popped = 0;
    do {
      popped = apu->popSamples(audioScratch.data(), audioScratch.size());
    } while (popped == audioScratch.size());
  }

  Result checkResult() {
    if (protocol_ == Protocol::Memory) {
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
        protocol_ == Protocol::Mooneye ? MOONEYE_PASS : PASS_MARKER;
    const std::string_view fail =
        protocol_ == Protocol::Mooneye ? MOONEYE_FAIL : FAIL_MARKER;

    if (serialLog.contains(pass)) {
      return Result::Pass;
    }
    if (serialLog.contains(fail)) {
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
    if (protocol_ != Protocol::Mooneye) {
      std::cout << serialLog << '\n';
      return;
    }
    std::cout << "serial:";
    for (char ch : serialLog) {
      auto byte = static_cast<unsigned char>(ch);
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
  std::unique_ptr<StateSerializer> stateSerializer;

  Protocol protocol_ = Protocol::Blargg;
  std::string serialLog;
  bool serialDirty = false;
  bool frameDirty = false;
  uint64_t totalCycles = 0;
  uint32_t framesElapsed = 0;
  std::array<StereoSample, AUDIO_DRAIN_CHUNK> audioScratch{};
  std::vector<InputEvent> input;
};

struct BenchOptions {
  uint32_t frames = DEFAULT_BENCH_FRAMES;
  uint32_t runs = DEFAULT_BENCH_RUNS;
  uint32_t warmup = DEFAULT_BENCH_WARMUP;
};

struct BenchSample {
  double seconds = 0.0;
  uint64_t hash = 0;
};

std::string formatHash(uint64_t hash) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

BenchSample timeOneRun(const std::string& romPath, uint64_t mcycles) {
  TestRunner runner(romPath, Protocol::Blargg);
  const auto start = std::chrono::steady_clock::now();
  runner.runCycles(mcycles);
  const auto stop = std::chrono::steady_clock::now();
  return {.seconds = std::chrono::duration<double>(stop - start).count(),
          .hash = hashFramebuffer(runner.videoBuffer())};
}

int runBenchmark(const std::string& romPath, const BenchOptions& options) {
  const uint64_t mcycles =
      static_cast<uint64_t>(options.frames) * MCYCLES_PER_FRAME;
  const auto fps = [&options](double seconds) {
    return static_cast<double>(options.frames) / seconds;
  };

  std::cout << std::fixed;
  uint64_t expectedHash = 0;
  std::vector<double> times;
  times.reserve(options.runs);
  const uint32_t total = options.warmup + options.runs;
  for (uint32_t i = 0; i < total; i++) {
    const BenchSample sample = timeOneRun(romPath, mcycles);
    if (i == 0) {
      expectedHash = sample.hash;
    } else if (sample.hash != expectedHash) {
      std::cerr << "ERROR: framebuffer hash changed between runs ("
                << formatHash(expectedHash) << " vs " << formatHash(sample.hash)
                << ")\n";
      return 1;
    }

    const bool isWarmup = i < options.warmup;
    const uint32_t label = isWarmup ? i + 1 : i - options.warmup + 1;
    std::cout << (isWarmup ? "warmup " : "run    ") << std::setw(2) << label
              << ": " << std::setprecision(1) << std::setw(10)
              << fps(sample.seconds) << " fps  (" << std::setprecision(4)
              << sample.seconds << " s)  hash " << formatHash(sample.hash)
              << '\n';
    if (!isWarmup) {
      times.push_back(sample.seconds);
    }
  }

  std::ranges::sort(times);
  const auto discard = static_cast<std::size_t>(
      std::sqrt(static_cast<double>(times.size())) / 2.0);
  const std::span<const double> kept =
      std::span(times).subspan(discard, times.size() - (2 * discard));

  double sum = 0.0;
  for (double t : kept) {
    sum += t;
  }
  const double mean = sum / static_cast<double>(kept.size());
  double squares = 0.0;
  for (double t : kept) {
    squares += (t - mean) * (t - mean);
  }
  const double relativeStddev =
      std::sqrt(squares / static_cast<double>(kept.size())) / mean;

  const std::size_t mid = times.size() / 2;
  const double median =
      times.size() % 2 == 0 ? (times[mid - 1] + times[mid]) / 2.0 : times[mid];

  std::cout << "\nframes " << options.frames << ", runs " << options.runs
            << " (dropped " << discard << " from each end), warmup "
            << options.warmup << '\n'
            << "median        " << std::setprecision(1) << fps(median)
            << " fps\n"
            << "trimmed mean  " << fps(mean) << " fps  +/- "
            << std::setprecision(2) << relativeStddev * 100.0 << "%\n";

  if (relativeStddev > MAX_RELATIVE_STDDEV) {
    std::cerr << "WARNING: stddev above 3%; close other apps or raise "
                 "--bench-runs\n";
  }
  return 0;
}

std::string saveBody(TestRunner& runner) {
  std::stringstream out;
  if (!runner.serializer().save(out)) {
    throw std::runtime_error("save failed");
  }
  const std::string data = out.str();
  constexpr std::size_t HEADER_LENGTH_OFFSET = 8;
  uint32_t headerLength = 0;
  std::memcpy(&headerLength, data.data() + HEADER_LENGTH_OFFSET,
              sizeof(headerLength));
  return data.substr(HEADER_LENGTH_OFFSET + sizeof(headerLength) +
                     headerLength);
}

std::vector<std::pair<std::string, std::string>> splitSections(
    const std::string& body) {
  std::vector<std::pair<std::string, std::string>> sections;
  std::size_t pos = sizeof(uint32_t);
  while (pos < body.size()) {
    const auto nameLength = static_cast<uint8_t>(body[pos]);
    pos += 1;
    std::string name = body.substr(pos, nameLength);
    pos += nameLength;
    uint32_t size = 0;
    std::memcpy(&size, body.data() + pos, sizeof(size));
    pos += sizeof(size);
    sections.emplace_back(std::move(name), body.substr(pos, size));
    pos += size;
  }
  return sections;
}

bool reportDivergence(const std::string& expected, const std::string& actual,
                      std::string_view context) {
  const auto expectedSections = splitSections(expected);
  const auto actualSections = splitSections(actual);
  bool diverged = expectedSections.size() != actualSections.size();
  for (std::size_t i = 0;
       i < std::min(expectedSections.size(), actualSections.size()); i++) {
    if (expectedSections[i] != actualSections[i]) {
      std::cerr << "FAIL: " << context << ": section "
                << expectedSections[i].first << " differs\n";
      diverged = true;
    }
  }
  return diverged;
}

bool reportFrameDivergence(const TestRunner& original,
                           const TestRunner& restored,
                           std::string_view context) {
  const uint64_t originalHash = hashFramebuffer(original.videoBuffer());
  const uint64_t restoredHash = hashFramebuffer(restored.videoBuffer());
  if (originalHash == restoredHash) {
    return false;
  }
  std::cerr << "FAIL: " << context << " (" << formatHash(originalHash) << " vs "
            << formatHash(restoredHash) << ")\n";
  return true;
}

int runSaveStateTest(const std::string& romPath,
                     std::optional<uint8_t> drawingLine) {
  TestRunner original(romPath, Protocol::Blargg);
  original.runFrames(SAVESTATE_WARMUP_FRAMES);
  if (drawingLine &&
      !original.runUntilDrawing(*drawingLine, SAVESTATE_DRAWING_STEPS,
                                SAVESTATE_WARMUP_FRAMES + 1)) {
    std::cerr << "FAIL: never reached mode 3 on line "
              << static_cast<int>(*drawingLine) << "\n";
    return 1;
  }
  std::stringstream snapshot;
  if (!original.serializer().save(snapshot)) {
    std::cerr << "FAIL: save failed\n";
    return 1;
  }
  TestRunner restored(romPath, Protocol::Blargg);
  if (!restored.serializer().load(snapshot)) {
    std::cerr << "FAIL: load failed\n";
    return 1;
  }

  original.runFrames(SAVESTATE_WARMUP_FRAMES + 1);
  restored.runFrames(1);
  if (reportFrameDivergence(original, restored,
                            "first frame after load diverged")) {
    return 1;
  }

  original.runFrames(SAVESTATE_WARMUP_FRAMES + SAVESTATE_COMPARE_FRAMES);
  restored.runFrames(SAVESTATE_COMPARE_FRAMES);
  if (reportFrameDivergence(original, restored,
                            "framebuffer diverged after load")) {
    return 1;
  }
  if (reportDivergence(saveBody(original), saveBody(restored),
                       "state diverged after load")) {
    return 1;
  }

  const std::string bytes = snapshot.str();
  std::stringstream truncated(bytes.substr(0, bytes.size() / 2));
  const std::string beforeFailedLoad = saveBody(restored);
  if (restored.serializer().load(truncated)) {
    std::cerr << "FAIL: truncated save state loaded\n";
    return 1;
  }
  if (reportDivergence(beforeFailedLoad, saveBody(restored),
                       "failed load changed state")) {
    return 1;
  }

  std::cout << "Passed\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string romPath;
    uint64_t budget = DEFAULT_BUDGET_MCYCLES;
    Protocol protocol = Protocol::Blargg;
    uint32_t frames = 0;
    bool frameMode = false;
    bool printHash = false;
    std::string dumpPath;
    bool benchMode = false;
    bool allowUnoptimized = false;
    bool saveStateMode = false;
    std::optional<uint8_t> saveStateLine;
    std::vector<InputEvent> input;
    BenchOptions bench;

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
      } else if (arg == "--bench") {
        benchMode = true;
      } else if (arg == "--bench-frames") {
        bench.frames = static_cast<uint32_t>(std::stoul(argv[++i]));
      } else if (arg == "--bench-runs") {
        bench.runs = static_cast<uint32_t>(std::stoul(argv[++i]));
      } else if (arg == "--bench-warmup") {
        bench.warmup = static_cast<uint32_t>(std::stoul(argv[++i]));
      } else if (arg == "--savestate") {
        saveStateMode = true;
      } else if (arg == "--savestate-line") {
        saveStateMode = true;
        const auto line = std::stoul(argv[++i]);
        if (line >= VISIBLE_LINES) {
          std::cerr << "--savestate-line must be below "
                    << static_cast<int>(VISIBLE_LINES) << "\n";
          return 3;
        }
        saveStateLine = static_cast<uint8_t>(line);
      } else if (arg == "--input") {
        input = parseInput(argv[++i]);
      } else if (arg == "--allow-unoptimized") {
        allowUnoptimized = true;
      } else if (romPath.empty()) {
        romPath = arg;
      } else {
        budget = std::stoull(arg) * 1'000'000ULL;
      }
    }

    if (romPath.empty()) {
      std::cerr << "Usage: test_runner <rom_path> [max_million_cycles] "
                   "[--mooneye] [--memory] [--frames N] [--hash] "
                   "[--dump path] [--input FRAME:BUTTON,...] [--bench "
                   "[--bench-frames N] "
                   "[--bench-runs R] [--bench-warmup W] "
                   "[--allow-unoptimized]] "
                   "[--savestate | --savestate-line LY]\n";
      return 3;
    }

    if (benchMode) {
      if (bench.frames == 0 || bench.runs == 0 || bench.runs > MAX_BENCH_RUNS ||
          bench.warmup > MAX_BENCH_RUNS) {
        std::cerr << "ERROR: --bench-frames must be > 0, --bench-runs 1-"
                  << MAX_BENCH_RUNS << ", --bench-warmup 0-" << MAX_BENCH_RUNS
                  << '\n';
        return 3;
      }
      if (!allowUnoptimized && !OPTIMIZED_BUILD) {
        std::cerr << "ERROR: built without optimizations; configure with "
                     "-DCMAKE_BUILD_TYPE=Release or pass "
                     "--allow-unoptimized\n";
        return 3;
      }
      if (!ASSERTS_DISABLED) {
        std::cerr << "WARNING: NDEBUG is not defined; assertions are on\n";
      }
      return runBenchmark(romPath, bench);
    }

    if (saveStateMode) {
      return runSaveStateTest(romPath, saveStateLine);
    }

    TestRunner runner(romPath, protocol);
    runner.setInput(std::move(input));

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
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 3;
  }
}
