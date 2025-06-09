#pragma once
#include "evite_policy.h"
#include <iostream>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <vector>
namespace fast_cache {

template <typename K, typename V> class CacheShard;

static const int defaultShardNum = 256;

// 1G大小
static const int64_t defaultCapacitySize = 1024 * 1024 * 1024;

template <typename K, typename V> class FastCache {
public:
  FastCache(const int shard_num = defaultShardNum,
            const int64_t capacity_size = defaultCapacitySize,
            const DataEvitePolicy evite_policy = LRU);
  ~FastCache() ;

  bool set(const K &key, const V &val);
  int get_shard_index(const K &key);
  bool set(const K &key, const V &val, int64_t expire_time);
  bool get(const K &key, V &val);
  bool del(const K &key);

  int64_t getCurrentTs();

private:
  std::vector<std::unique_ptr<CacheShard<K, V>>> _cache_shards;
  std::vector<std::unique_ptr<std::shared_mutex>> _shard_mutexs;
  int _shard_num;

  // 更新系统时间戳的异步线程
  void startSystemCurrentTsUpdater();
  void stopSystemCurrentTsUpdater();
  std::atomic<int64_t> current_ts_;
  std::atomic<bool> is_running_;
  std::thread current_ts_thread_;
};

} // namespace fast_cache

#include "fast_cache.hpp"