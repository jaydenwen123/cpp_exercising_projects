#pragma once
#include "cache_shard.h"
#include "evite_policy.h"
#include "fast_cache.h"
#include "logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <atomic>

namespace fast_cache {

template <typename K, typename V>
inline FastCache<K, V>::FastCache(const int shard_num,
                                  const int64_t capacity_size,
                                  const DataEvitePolicy evite_policy) {
  _shard_num = shard_num;
  _cache_shards.reserve(shard_num);
  _shard_mutexs.reserve(shard_num);
  int64_t shard_capacity = capacity_size / shard_num;
  for (int i = 0; i < shard_num; i++) {
    // 这里根据指定的容量大小初始化每个cache_shards的大小
    _cache_shards.emplace_back(
        std::make_unique<CacheShard<K, V>>(shard_capacity,this, evite_policy));
    _shard_mutexs.emplace_back(std::make_unique<std::shared_mutex>());
  }
}

template <typename K, typename V> FastCache<K, V>::~FastCache() {
  stopSystemCurrentTsUpdater();
}

template <typename K, typename V>
inline bool FastCache<K, V>::set(const K &key, const V &val) {
  int shard_index = get_shard_index(key);
  // 第三步再调用每个_cache_shards的set方法
  std::unique_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->set(key, val);
}

template <typename K, typename V>
inline int FastCache<K, V>::get_shard_index(const K &key) {
  // 第一步：先计算key的哈希值
  std::hash<K> hasher;
  size_t hash_value = hasher(key);
  // 第二步：根据哈希值计算分到哪个哈希槽
  int shard_index = hash_value % _shard_num;
  return shard_index;
}

template <typename K, typename V>
inline bool FastCache<K, V>::set(const K &key, const V &val,
                                 int64_t expire_time) {
  int shard_index = get_shard_index(key);
  std::unique_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->set(key, val, expire_time);
}

template <typename K, typename V>
inline bool FastCache<K, V>::get(const K &key, V &val) {
  int shard_index = get_shard_index(key);
  std::unique_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->get(key, val);
}

template <typename K, typename V>
inline bool FastCache<K, V>::del(const K &key) {
  int shard_index = get_shard_index(key);
  std::unique_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->del(key);
}

template <typename K, typename V>
inline int64_t FastCache<K, V>::getCurrentTs() {
  return current_ts_.load(std::memory_order_relaxed);
}

template <typename K, typename V>
inline void FastCache<K, V>::startSystemCurrentTsUpdater() {
  is_running_ = true;
  current_ts_thread_ = std::thread([&]() {
    while (is_running_) {
      auto current_ts = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
      current_ts_.store(current_ts, std::memory_order_release);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });
}

template <typename K, typename V>
inline void FastCache<K, V>::stopSystemCurrentTsUpdater() {
  is_running_ = false;
  if (current_ts_thread_.joinable()) {
    current_ts_thread_.join();
  }
}

} // namespace fast_cache