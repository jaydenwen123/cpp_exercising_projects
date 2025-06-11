#ifndef LOGGER_H
#define LOGGER_H
#include "spdlog/spdlog.h"
#include <spdlog/sinks/basic_file_sink.h>
namespace fast_cache {
    void initLogger();
    using spdlog::critical;
    using spdlog::debug;
    using spdlog::error;
    using spdlog::info;
    using spdlog::trace;
    using spdlog::warn;
} // namespace fast_cache
#endif // LOGGER_H