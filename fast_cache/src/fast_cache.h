#include "cache_shard.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <iostream>
#include <memory>
#include <shared_mutex>
#include <vector>
#include "logger.h"
namespace fast_cache {

static const int kShardNum = 256;

// 1G大小
static const int64_t capacityDefaultSize = 1024 * 1024 * 1024;

template <typename K, typename V> class FastCache {
public:
  FastCache(const int shard_num = kShardNum,
            const int64_t capacity_size = capacityDefaultSize);
  ~FastCache() {}

  bool set(const K &key, const V &val);
  int get_shard_index(const K &key);
  bool set(const K &key, const V &val, int64_t expire_time);
  bool get(const K &key, V &val);
  bool del(const K &key);

private:
  std::vector<std::unique_ptr<CacheShard<K, V>>> _cache_shards;
  std::vector<std::unique_ptr<std::shared_mutex>> _shard_mutexs;
  int _shard_num;
};

template <typename K, typename V>
inline FastCache<K, V>::FastCache(const int shard_num,
                                  const int64_t capacity_size) {
  _shard_num = shard_num;
  _cache_shards.reserve(shard_num);
  _shard_mutexs.reserve(shard_num);
  int64_t shard_size = capacity_size / shard_num;
  for (int i = 0; i < shard_num; i++) {
    // 这里根据指定的容量大小初始化每个cache_shards的大小
    _cache_shards.emplace_back(std::make_unique<CacheShard<K, V>>(shard_size));
    _shard_mutexs.emplace_back(std::make_unique<std::shared_mutex>());
  }
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
  std::shared_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->get(key, val);
}

template <typename K, typename V>
inline bool FastCache<K, V>::del(const K &key) {
  int shard_index = get_shard_index(key);
  std::unique_lock<std::shared_mutex> lock(*_shard_mutexs[shard_index]);
  return _cache_shards[shard_index]->del(key);
}

} // namespace fast_cache