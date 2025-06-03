#include "fast_cache.h"
#include <benchmark/benchmark.h>
#include <unordered_map>
#include <string>
using namespace fast_cache;
// 初始化 FastCache
static void BM_FastCache_Set(benchmark::State& state) {
    FastCache<std::string, std::string> cache;
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            cache.set(key, value);
        }
    }
}

// 测试 FastCache 的 get 操作
static void BM_FastCache_Get(benchmark::State& state) {
    FastCache<std::string, std::string> cache;
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
static void BM_UnorderedMap_Set(benchmark::State& state) {
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
static void BM_UnorderedMap_Get(benchmark::State& state) {
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
BENCHMARK(BM_FastCache_Set)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_FastCache_Get)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_UnorderedMap_Set)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK(BM_UnorderedMap_Get)->Arg(10000)->Arg(100000)->Arg(1000000);
BENCHMARK_MAIN();