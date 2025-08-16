#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>

class Logger
{
public:
  static std::shared_ptr<spdlog::logger> GetLogger()
  {
    static std::shared_ptr<spdlog::logger> logger = nullptr;

    if (!logger)
    {
      spdlog::init_thread_pool(8192, 1);

      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_level(spdlog::level::info);

      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("../logs/gameboy.log", true);

      logger = std::make_shared<spdlog::async_logger>(
          "CORE",
          spdlog::sinks_init_list{console_sink, file_sink},
          spdlog::thread_pool());

      spdlog::register_logger(logger);
      logger->set_level(spdlog::level::trace);
      logger->flush_on(spdlog::level::trace);

      static struct LoggerCleanup
      {
        ~LoggerCleanup() { spdlog::shutdown(); }
      } cleanup;
    }

    return logger;
  }

private:
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
};