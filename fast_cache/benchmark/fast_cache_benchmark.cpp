#include "fast_cache.h"
#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
using namespace fast_cache;

static void BM_FastCache_Set_Random(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);
      cache.set(key, value);
    }
  }
}

// 测试 FastCache 的 get 操作
static void BM_FastCache_Get_Random(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  ;
  for (int i = 0; i < state.range(0); ++i) {
    std::string key = "key_" + std::to_string(i);
    std::string value = "value_" + std::to_string(i);
    cache.set(key, value);
  }
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value;
      cache.get(key, value);
    }
  }
}

static void BM_FastCache_Set_LFU(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);
      cache.set(key, value);
    }
  }
}

// 测试 FastCache 的 get 操作
static void BM_FastCache_Get_LFU(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  ;
  for (int i = 0; i < state.range(0); ++i) {
    std::string key = "key_" + std::to_string(i);
    std::string value = "value_" + std::to_string(i);
    cache.set(key, value);
  }
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value;
      cache.get(key, value);
    }
  }
}

// 初始化 FastCache
static void BM_FastCache_Set_LRU(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);
      cache.set(key, value);
    }
  }
}

// 测试 FastCache 的 get 操作
static void BM_FastCache_Get_LRU(benchmark::State &state) {
  FastCache<std::string, std::string> cache(defaultShardNum,
                                            defaultCapacitySize, LFU);
  ;
  for (int i = 0; i < state.range(0); ++i) {
    std::string key = "key_" + std::to_string(i);
    std::string value = "value_" + std::to_string(i);
    cache.set(key, value);
  }
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value;
      cache.get(key, value);
    }
  }
}

// 测试 std::unordered_map 的 set 操作
static void BM_UnorderedMap_Set(benchmark::State &state) {
  std::unordered_map<std::string, std::string> map;
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      std::string value = "value_" + std::to_string(i);
      map[key] = value;
    }
  }
}

// 测试 std::unordered_map 的 get 操作
static void BM_UnorderedMap_Get(benchmark::State &state) {
  std::unordered_map<std::string, std::string> map;
  for (int i = 0; i < state.range(0); ++i) {
    std::string key = "key_" + std::to_string(i);
    std::string value = "value_" + std::to_string(i);
    map[key] = value;
  }
  for (auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      std::string key = "key_" + std::to_string(i);
      auto it = map.find(key);
      if (it != map.end()) {
        std::string value = it->second;
      }
    }
  }
}

// 注册基准测试
BENCHMARK(BM_FastCache_Set_LRU)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_FastCache_Set_LFU)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_FastCache_Set_Random)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_UnorderedMap_Set)->Arg(10000)->Arg(100000)->Arg(1000000);

BENCHMARK(BM_FastCache_Get_LRU)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_FastCache_Get_LFU)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_FastCache_Get_Random)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_UnorderedMap_Get)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK_MAIN();