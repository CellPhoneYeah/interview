#ifndef SLOG_H
#define SLOG_H
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <memory>
inline void setup_logger(){
    static bool initialized = false;
    if (!initialized)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto logger = std::make_shared<spdlog::logger>("console", console_sink);

        logger->set_pattern("%Y-%m-%d %H:%M:%S [%^%l%$] %@ %! - %v");

        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        initialized = true;
    }
}
namespace {
    struct InitialSpdlog{
        InitialSpdlog(){
            setup_logger();
        }
    };
    static InitialSpdlog initialSpdlog;
}
#endif