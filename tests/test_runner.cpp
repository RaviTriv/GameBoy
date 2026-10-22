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
#include "Common.h"
#include "Lcd.h"
#include "Logger.h"
#include "Machine.h"
#include "StateSerializer.h"

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

enum class Protocol { Blargg, Mooneye, Memory, Registers, Hram };

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

constexpr uint8_t OPCODE_LD_B_B = 0x40;
constexpr uint8_t OPCODE_LEGACY_MOONEYE_EXIT = 0xED;
constexpr std::array<uint8_t, 6> FIBONACCI_SIGNATURE = {3, 5, 8, 13, 21, 34};
constexpr uint32_t BREAK_SETTLE_FRAMES = 2;

constexpr uint16_t HRAM_RESULT_ADDR = 0xFF82;
constexpr uint8_t HRAM_PASS_CODE = 0x01;
constexpr uint8_t HRAM_FAIL_CODE = 0xFF;

class TestRunner {
 public:
  TestRunner(const std::string& romPath, Protocol protocol)
      : protocol_(protocol) {
    Logger::Disable();
    machine = std::make_unique<Machine>(romPath);
    machine->setSerialSink([this](uint8_t byte) {
      serialLog.push_back(static_cast<char>(byte));
      serialDirty = true;
    });
    machine->setOnFrameComplete([this] {
      frameDirty = true;
      framesElapsed++;
      drainAudio();
      applyInput();
    });
  }

  int run(uint64_t budget) {
    if (protocol_ == Protocol::Registers) {
      return runUntilSignature(budget);
    }
    while (machine->cycles() < budget) {
      machine->step();
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
      machine->step();
    }
  }

  [[nodiscard]] bool runUntilDrawing(uint8_t line, uint32_t extraSteps,
                                     uint32_t maxFrames) {
    while (machine->lcd().getLy() != line ||
           machine->lcd().getLcdMode() != LCD::DRAWING) {
      if (framesElapsed >= maxFrames) {
        return false;
      }
      machine->step();
    }
    for (uint32_t i = 0;
         i < extraSteps && machine->lcd().getLcdMode() == LCD::DRAWING; i++) {
      machine->step();
    }
    return machine->lcd().getLy() == line &&
           machine->lcd().getLcdMode() == LCD::DRAWING;
  }

  void runCycles(uint64_t mcycles) {
    while (machine->cycles() < mcycles) {
      machine->step();
    }
  }

  void runUntilBreak(uint64_t mcycles) {
    while (machine->cycles() < mcycles) {
      const uint8_t opcode = currentOpcode();
      machine->step();
      if (opcode == OPCODE_LD_B_B) {
        runCycles(machine->cycles() +
                  (BREAK_SETTLE_FRAMES * MCYCLES_PER_FRAME));
        return;
      }
    }
  }

  int checkHramResult() {
    const uint8_t code = machine->memory().read8(HRAM_RESULT_ADDR);
    std::cout << "$FF82: " << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(code) << std::dec << '\n';
    if (code == HRAM_PASS_CODE) {
      return 0;
    }
    if (code == HRAM_FAIL_CODE) {
      return 1;
    }
    std::cerr << "no result written to $FF82\n";
    return 2;
  }

  [[nodiscard]] const auto& videoBuffer() const {
    return machine->ppu().getPublishedFrame();
  }

  [[nodiscard]] StateSerializer& serializer() { return machine->serializer(); }

  void setInput(std::vector<InputEvent> events) { input = std::move(events); }

 private:
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
        machine->gamepad().setAPressed(pressed);
        break;
      case 'B':
        machine->gamepad().setBPressed(pressed);
        break;
      case 'S':
        machine->gamepad().setStartPressed(pressed);
        break;
      case 'E':
        machine->gamepad().setSelectPressed(pressed);
        break;
      case 'U':
        machine->gamepad().setUpPressed(pressed);
        break;
      case 'D':
        machine->gamepad().setDownPressed(pressed);
        break;
      case 'L':
        machine->gamepad().setLeftPressed(pressed);
        break;
      case 'R':
        machine->gamepad().setRightPressed(pressed);
        break;
      default:
        throw std::invalid_argument(std::string("Unknown button: ") + button);
    }
  }

  void drainAudio() {
    std::size_t popped = 0;
    do {
      popped = machine->apu().popSamples(audioScratch);
    } while (popped == audioScratch.size());
  }

  uint8_t currentOpcode() {
    return machine->memory().read8(machine->cpu().registers().pc);
  }

  int runUntilSignature(uint64_t budget) {
    while (machine->cycles() < budget) {
      const uint8_t opcode = currentOpcode();
      if (opcode == OPCODE_LEGACY_MOONEYE_EXIT) {
        return reportRegisters();
      }
      machine->step();
      if (opcode == OPCODE_LD_B_B) {
        return reportRegisters();
      }
    }
    std::cerr << "TIMEOUT after " << budget << " cycles\n";
    return 2;
  }

  int reportRegisters() const {
    const Registers& r = machine->cpu().registers();
    const std::array<uint8_t, 6> actual = {r.b, r.c, r.d, r.e, r.h, r.l};
    std::cout << "B=" << +r.b << " C=" << +r.c << " D=" << +r.d << " E=" << +r.e
              << " H=" << +r.h << " L=" << +r.l << '\n';
    return actual == FIBONACCI_SIGNATURE ? 0 : 1;
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
    if (machine->memory().read8(MEMORY_SIGNATURE_ADDR) != MEMORY_SIGNATURE_0 ||
        machine->memory().read8(MEMORY_SIGNATURE_ADDR + 1) !=
            MEMORY_SIGNATURE_1 ||
        machine->memory().read8(MEMORY_SIGNATURE_ADDR + 2) !=
            MEMORY_SIGNATURE_2) {
      return Result::Running;
    }

    const uint8_t code = machine->memory().read8(MEMORY_RESULT_ADDR);
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

  std::unique_ptr<Machine> machine;

  Protocol protocol_ = Protocol::Blargg;
  std::string serialLog;
  bool serialDirty = false;
  bool frameDirty = false;
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
    bool untilBreak = false;
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
      } else if (arg == "--regs") {
        protocol = Protocol::Registers;
      } else if (arg == "--ff82") {
        protocol = Protocol::Hram;
      } else if (arg == "--until-break") {
        untilBreak = true;
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
                   "[--mooneye] [--memory] [--regs] [--ff82 --frames N] "
                   "[--frames N] [--until-break] [--hash] "
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

    if (protocol == Protocol::Hram) {
      if (!frameMode) {
        std::cerr << "--ff82 needs --frames N\n";
        return 3;
      }
      runner.runCycles(frames * MCYCLES_PER_FRAME);
      return runner.checkHramResult();
    }

    if (frameMode) {
      if (untilBreak) {
        runner.runUntilBreak(frames * MCYCLES_PER_FRAME);
      } else {
        runner.runFrames(frames);
      }
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
