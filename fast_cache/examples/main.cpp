#include "cache_shard.h"
#include <chrono>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

// 测试用例1：基本set/get功能
void test_basic_operations(fast_cache::CacheShard<std::string, std::string> &cache) {
  spdlog::info("=== 开始基础功能测试 ===");

  // 测试正常插入和查询
  if (cache.set("key1", "value1")) {
    std::string val;
    if (cache.get("key1", val)) {
      spdlog::info("测试通过: key1 -> {}", val);
    } else {
      spdlog::error("测试失败: 无法获取key1");
    }
  }

  // 测试不存在的key
  std::string dummy;
  if (!cache.get("non_exist", dummy)) {
    spdlog::info("测试通过: 不存在的key返回false");
  }
}

// 测试用例2：删除功能
void test_delete_operation(
    fast_cache::CacheShard<std::string, std::string> &cache) {
  spdlog::info("\n=== 开始删除功能测试 ===");

  cache.set("to_delete", "delete_me");
  if (cache.del("to_delete")) {
    std::string val;
    if (!cache.get("to_delete", val)) {
      spdlog::info("测试通过: 删除后查询返回false");
    }
  }
}

// 测试用例3：过期功能
void test_expiration(fast_cache::CacheShard<std::string, std::string> &cache) {
  spdlog::info("\n=== 开始过期测试 ===");

  // 设置3秒过期的数据
  cache.set("temp_key", "temp_value", 3);

  std::string val;
  if (cache.get("temp_key", val)) {
    spdlog::info("测试通过: 过期前能获取数据");
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  if (!cache.get("temp_key", val)) {
    spdlog::info("测试通过: 过期后返回false");
  }
}

// 测试用例4：容量限制
void test_capacity(fast_cache::CacheShard<std::string, std::string> &cache) {
  spdlog::info("\n=== 开始容量测试 ===");

  std::string large_value(1024 * 1024, 'a'); // 1MB数据
  for (int i = 0; i < 150; i++) {
    if (!cache.set("big_" + std::to_string(i), large_value)) {
      spdlog::info("测试通过: 达到容量限制后插入失败");
      break;
    }
  }
}

int main() {
  spdlog::set_level(spdlog::level::debug);

  // 初始化缓存（100MB容量）
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);

  // 执行测试用例
  test_basic_operations(cache);
  test_delete_operation(cache);
  test_expiration(cache);
  test_capacity(cache);

  spdlog::info("\n=== 所有测试完成 ===");
  return 0;
}