#include "logger.h"
#include <iostream>
#include <string>

namespace fast_cache {

 void initLogger() {
  /* try {
    // 创建日志记录器
    auto logger = spdlog::default_logger;
        // spdlog::basic_logger_mt("fast_cache_logger", "fast_cache.log");
    // 设置日志格式
    logger.set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    // 设置日志级别
    logger->set_level(spdlog::level::debug);
    // 设置日志输出到文件
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  } */
};
} // namespace fast_cache