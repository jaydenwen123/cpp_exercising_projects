#pragma once
#include "cache_item.h"
#include "evite_policy.h"
#include "fast_cache.h"
#include "serializer.h"
#include <iterator>
#include <list>
#include <map>
#include <mutex>
#include <random>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <zlib.h>
namespace fast_cache {

enum DataOpType {
  SET,
  GET,
  DEL,
  // todo...
};
// 存储索引信息
struct ItemIndex {
  int64_t start;
  int64_t length;
  int64_t expire_time;
};

template <typename K> struct IndexEntry {
  ItemIndex index;
  typename std::list<std::pair<K, ItemIndex>>::iterator lru_iter = {};
  // std::pair<int64_t,int64_t>中，第一个是访问次数，第二个访问时间戳
  std::pair<int64_t, int64_t> lfu_count = {};
};

// 分片shard，现成不安全
template <typename K, typename V> class CacheShard {
public:
  CacheShard(const int64_t capacity, FastCache<K, V> *parent = nullptr,
             const DataEvitePolicy evite_policy = LRU)
      : parent_(parent), evite_policy_(evite_policy) {
    init(capacity);
    // 开始启动异步任务
    /* data_merger_thread_ = std::thread(
        [this]() {
          while (!stop_flag_.load(std::memory_order_relaxed)) {
            if (need_evite_) {
              spdlog::info("need merge data");
              this->backendDataMergeTasker();
              this->need_evite_.store(false);
            }
            std::this_thread::sleep_for(
                std::chrono::milliseconds(data_evite_interval_));
            if (!need_evite_) {
              this->need_evite_.store(true, std::memory_order_relaxed);
            }
          }
        },
        this); */
  }
  // 禁用拷贝构造和赋值
  CacheShard(const CacheShard &) = delete;
  CacheShard &operator=(const CacheShard &) = delete;
  ~CacheShard() {
    stop_flag_.store(true, std::memory_order_relaxed);
    if (data_merger_thread_.joinable()) {
      data_merger_thread_.join();
    }
  }
  // 初始化data_的容量
  void init(int64_t capacity);
  // 插入or 更新数据
  bool set(const K &key, const V &val);
  bool set(const K &key, const V &val, int64_t expire_time);
  bool set(CacheItem<K, V> &item);

  // 获取数据
  bool get(const K &key, V &val);

  // 删除数据
  bool del(const K &key);

  int64_t size();
  bool empty();
  void clear();

private:
  bool getIndex(const K &key, IndexEntry<K> &index_entry);
  bool itemIsExpired(int64_t expire_time);
  // 后台异步任务数据合并整理
  void backendDataMergeTasker();
  void updateStatInfo(IndexEntry<K> &index_entry);
  void updateIndex(CacheItem<K, V> &item, IndexEntry<K> &index_entry);
  void updateEvitInfo(const K &key, IndexEntry<K> &index_entry,
                      DataOpType op_type = SET, bool exist = true);
  void updateEvitInfoWithLRUStrategy(const K &key, IndexEntry<K> &index_entry,
                                     DataOpType op_type, bool exist);
  void updateEvitInfoWithLFUStrategy(const K &key, IndexEntry<K> &index_entry,
                                     DataOpType op_type, bool exist);
  void updateEvitInfoWithRandomStrategy(const K &key,
                                        IndexEntry<K> &index_entry,
                                        DataOpType op_type, bool exist);

  bool eviteItem(std::vector<uint8_t> &item_data);
  // 自动获取时间戳
  int64_t getSystemCurrentTs();
  // 写入data_
  bool writeIntoData(std::vector<uint8_t> &data);
  // 序列化
  void serialize(CacheItem<K, V> &item, std::vector<uint8_t> &data);
  // 反序列化
  bool deserialize(std::vector<uint8_t> &data, CacheItem<K, V> &item);

  int64_t cal_check_sum(std::vector<uint8_t> &data, int offset, int length);

  std::unordered_map<K, IndexEntry<K>> indexs_;
  // todo
  // 这里应该采用一个数据容器，比如std::array<uint8_t>。后续应该替换为一个循环队列。
  std::vector<uint8_t> data_;
  // 容量
  int64_t capacity_;

  // 数据淘汰策略
  DataEvitePolicy evite_policy_;
  // 支持LRU--数据的读写时更新lru_list_
  std::list<std::pair<K, ItemIndex>> lru_list_;
  // std::unordered_map<K, std::pair<int64_t, int64_t>> lfu_count_map_;
  // 支持LFU--数据读写时更新访问频率,支持有序
  // 按照访问频率进行排序
  std::multimap<std::pair<int64_t, int64_t>, K, LFUComparer> lfu_map_;

  // todo 支持数据清理/延迟删除
  // 统计数据空间的占比，当大于一定比例时出发数据清理逻辑
  std::atomic<bool> need_evite_;
  // 数据剩余空间,实际占用除以容量
  int64_t data_used_size_;
  // 无用数据(数据过期or删除)占用空间的比例，当大于一定比例时触发数据清理逻辑
  std::atomic<int64_t> data_invalid_size_;
  // 无效数据的比例阈值
  float data_evite_invalid_rate_threahold_;
  // 数据定时整理的时间间隔
  int64_t data_evite_interval_;
  // todo 支持数据持久化

  // todo 支持数据压缩

  FastCache<K, V> *parent_;
  std::shared_mutex shard_mutex_;
  std::thread data_merger_thread_;
  std::atomic<bool> stop_flag_{false};
};

template <typename K, typename V>
inline void CacheShard<K, V>::updateStatInfo(IndexEntry<K> &index_entry) {
  data_invalid_size_.fetch_add(index_entry.index.length,
                               std::memory_order_relaxed);
  if (data_invalid_size_.load(std::memory_order_relaxed) >
      capacity_ * data_evite_invalid_rate_threahold_) {
    need_evite_.store(true, std::memory_order_relaxed);
  }
}

template <typename K, typename V>
inline void CacheShard<K, V>::init(int64_t capacity) {
  data_.reserve(capacity);
  capacity_ = capacity;
  data_used_size_ = 0;
  data_invalid_size_ = 0;
  // todo移动到配置文件中
  data_evite_invalid_rate_threahold_ = 0.5;
  data_evite_interval_ = 1000;
  need_evite_ = false;
}

template <typename K, typename V>
inline bool CacheShard<K, V>::set(const K &key, const V &val) {
  CacheItem<K, V> item(key, val);
  return set(item);
}

template <typename K, typename V>
inline bool CacheShard<K, V>::set(const K &key, const V &val,
                                  int64_t expire_time) {
  CacheItem<K, V> item(key, val, expire_time);
  return set(item);
}

template <typename K, typename V>
inline bool CacheShard<K, V>::set(CacheItem<K, V> &item) {
  std::unique_lock<std::shared_mutex> lock(shard_mutex_);
  // 对item进行序列化
  std::vector<uint8_t> item_data;
  serialize(item, item_data);
  // 如果数据超过了size_，应该如何处理？
  if (data_.size() + item_data.size() > capacity_) {
    bool suc = eviteItem(item_data);
    if (!suc) {
      spdlog::error("eviteItem failed");
      return false;
    }
  }
  // 必须先保存，因为下面会把item_data的数据移动到data_中，所以直接取size()会是0
  // 写成功后再更新索引indexs_
  IndexEntry<K> index_entry;
  index_entry.index.start = data_.size();
  index_entry.index.length = item_data.size();
  index_entry.index.expire_time = item.expire_time;

  // 先写data_
  if (!writeIntoData(item_data)) {
    spdlog::error("writeIntoData failed");
    return false;
  }
  updateIndex(item, index_entry);
  return true;
}

template <typename K, typename V>
inline bool CacheShard<K, V>::eviteItem(std::vector<uint8_t> &item_data) {
  // 数据淘汰，然后再插入
  //  - LRU、LFU
  int64_t need_space_size = data_.size() + item_data.size() - capacity_;
  if (evite_policy_ == LRU) {
    // 如何淘汰呢？
    // 从lru的末尾移除一个元素(索引)
    while (need_space_size > 0) {
      auto &back = lru_list_.back();
      auto &key = back.first;
      auto &index = back.second;
      // 直接删除
      indexs_.erase(key);
      lru_list_.pop_back();
      need_space_size -= index.length;
    }
  } else if (evite_policy_ == LFU) {
    while (need_space_size > 0 && !lfu_map_.empty() && !indexs_.empty()) {
      auto min_iter = lfu_map_.begin();
      auto range_iter = lfu_map_.equal_range(min_iter->first);
      for (auto iter = range_iter.first;
           iter != range_iter.second && need_space_size > 0;) {
        auto &key = iter->second;
        auto &index_entry = indexs_[key];
        auto current_it = iter;
        iter++;
        indexs_.erase(key);
        lfu_map_.erase(current_it);
        need_space_size -= index_entry.index.length;
      }
    }
  } else if (evite_policy_ == RANDOM) {
    // 随机淘汰
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // 随机淘汰
    while (need_space_size > 0 && !indexs_.empty()) {
      int size = indexs_.size();
      std::uniform_real_distribution<> dis(0, size - 1);
      int get_random_index = dis(gen);
      auto iter = indexs_.begin();
      // 移动到随机为止
      std::advance(iter, get_random_index);
      // 下面方式会编译报错
      // iter += get_random_index;
      auto &key = iter->first;
      auto &index_entry = iter->second;
      indexs_.erase(key);
      need_space_size -= index_entry.index.length;
    }
  } else {
    return false;
  }
  return true;
}

template <typename K, typename V>
inline void CacheShard<K, V>::updateIndex(CacheItem<K, V> &item,
                                          IndexEntry<K> &index_entry) {

  bool exist = false;
  auto iter = indexs_.find(item.key);
  if (iter != indexs_.end()) {
    auto &old_index_entry = iter->second;
    auto latest_index = index_entry.index;
    // 这里通过 旧的index_entry赋值给新的index_entry，然后再更更新最新的index
    index_entry = old_index_entry;
    index_entry.index = latest_index;
    exist = true;
  }
  // 记录索引
  updateEvitInfo(item.key, index_entry, SET, exist);
  indexs_[item.key] = std::move(index_entry);
}

template <typename K, typename V>
inline void CacheShard<K, V>::updateEvitInfo(const K &key,
                                             IndexEntry<K> &index_entry,
                                             DataOpType op_type, bool exist) {
  if (evite_policy_ == LRU) {
    updateEvitInfoWithLRUStrategy(key, index_entry, op_type, exist);
  } else if (evite_policy_ == LFU) {
    // lfu
    updateEvitInfoWithLFUStrategy(key, index_entry, op_type, exist);
  } else {
    // 随机
    updateEvitInfoWithRandomStrategy(key, index_entry, op_type, exist);
  }
}
template <typename K, typename V>
inline void CacheShard<K, V>::updateEvitInfoWithLRUStrategy(
    const K &key, IndexEntry<K> &index_entry, DataOpType op_type, bool exist) {
  if (op_type == DEL && exist) {
    lru_list_.erase(index_entry.lru_iter);
  } else if (op_type == GET && exist) {
    lru_list_.splice(lru_list_.begin(), lru_list_, index_entry.lru_iter);
    index_entry.lru_iter = lru_list_.begin();
  } else {
    // SET
    // 存在的话先移除
    if (exist) {
      lru_list_.erase(index_entry.lru_iter);
    }
    lru_list_.push_front(std::make_pair(key, index_entry.index));
    index_entry.lru_iter = lru_list_.begin();
  }
}

template <typename K, typename V>
inline void CacheShard<K, V>::updateEvitInfoWithLFUStrategy(
    const K &key, IndexEntry<K> &index_entry, DataOpType op_type, bool exist) {
  int64_t current_ts = getSystemCurrentTs();
  // 实现LFU逻辑
  if (op_type == DEL && exist) {
    // 执行删除操作
    auto lfu_count = index_entry.lfu_count;
    auto range_iter = lfu_map_.equal_range(lfu_count);
    bool find = false;
    for (auto iter = range_iter.first; iter != range_iter.second; ++iter) {
      if (iter->second == key) {
        lfu_map_.erase(iter);
        find = true;
        break;
      }
    }
    if (!find) {
      spdlog::error("lfu_map_ {} not find key:{}", int(op_type), key);
    }
  } else if (op_type == GET && exist) {
    auto &lfu_count = index_entry.lfu_count;
    // 先找到，再删除
    auto range_iter = lfu_map_.equal_range(lfu_count);
    bool find = false;
    for (auto iter = range_iter.first; iter != range_iter.second; ++iter) {
      if (iter->second == key) {
        lfu_map_.erase(iter);
        // 只更新访问时间
        lfu_count.second = current_ts;
        lfu_map_.emplace(std::make_pair(lfu_count, key));
        find = true;
        break;
      }
    }
    if (!find) {
      spdlog::error("lfu_map_ {} not find key:{}", int(op_type), key);
    }
  } else {
    // SET
    if (exist) {
      // 先找到，再删除，再插入
      auto &lfu_count = index_entry.lfu_count;
      auto iter_range = lfu_map_.equal_range(lfu_count);
      bool find = false;
      // 先找到，再删除
      for (auto iter = iter_range.first; iter != iter_range.second; ++iter) {
        if (iter->second == key) {
          lfu_map_.erase(iter);
          lfu_count.first++;
          lfu_count.second = current_ts;
          lfu_map_.emplace(std::make_pair(lfu_count, key));
          find = true;
          break;
        }
      }
      if (!find) {
        spdlog::error("lfu_map_ {} not find key:{}", int(op_type), key);
      }
    } else {
      // 直接插入
      index_entry.lfu_count = std::move(std::make_pair(1, current_ts));
      lfu_map_.emplace(index_entry.lfu_count, key);
    }
  }
}

template <typename K, typename V>
inline void CacheShard<K, V>::updateEvitInfoWithRandomStrategy(
    const K &key, IndexEntry<K> &index_entry, DataOpType op_type, bool exist) {
  // 实现随机数逻辑
  // 随机策略下，数据读写的时候不需要更新，只有在淘汰时才需要处理
}

template <typename K, typename V>
inline bool CacheShard<K, V>::get(const K &key, V &val) {
  std::unique_lock<std::shared_mutex> lock(shard_mutex_);
  IndexEntry<K> index_entry;
  bool suc = getIndex(key, index_entry);
  if (!suc)
    return false;
  auto &index = index_entry.index;
  if (itemIsExpired(index.expire_time)) {
    spdlog::error("item is expired");
    // 更新无用数据的大小
    updateStatInfo(index_entry);
    return false;
  }
  //  然后再根据索引读取序列化的数据
  std::vector<uint8_t> data;
  data.reserve(index.length);
  auto start_iter = data_.begin() + index.start;
  auto end_iter = start_iter + index.length;
  //   todo这里进行拷贝，也可以尝试不拷贝，直接使用迭代器
  data.insert(data.begin(), start_iter, end_iter);
  // spdlog::info("data size:{},index start:{},length:{}", data.size(),
  //              index.start, index.length);
  //  接着对数据进行反序列化
  CacheItem<K, V> item;
  suc = deserialize(data, item);
  if (!suc) {
    spdlog::error("deserialize key:{} failed", key);
    return false;
  }

  if (item.expire_time != index.expire_time) {
    spdlog::error("item expire_time is not equal");
    updateStatInfo(index_entry);
    return false;
  }

  if (itemIsExpired(item.expire_time)) {
    spdlog::error("item is expired");
    // 更新无用数据的大小
    updateStatInfo(index_entry);
    return false;
  }

  val = item.val;
  //  最后返回数据
  return true;
}
template <typename K, typename V>
inline bool CacheShard<K, V>::getIndex(const K &key,
                                       IndexEntry<K> &entry_index) {
  //  先获取索引数据
  auto iter = indexs_.find(key);
  if (iter == indexs_.end()) {
    spdlog::error("key:{} is not found", key);
    return false;
  }
  entry_index = iter->second;
  updateEvitInfo(key, entry_index, GET);
  return true;
}
template <typename K, typename V>
inline bool CacheShard<K, V>::itemIsExpired(int64_t expire_time) {
  return expire_time != dataNoExpire && expire_time < getSystemCurrentTs();
}
template <typename K, typename V>
inline bool CacheShard<K, V>::del(const K &key) {
  std::unique_lock<std::shared_mutex> lock(shard_mutex_);
  auto iter = indexs_.find(key);
  if (iter == indexs_.end()) {
    spdlog::error("key:{} is not found", key);
    return false;
  }
  auto &index_entry = iter->second;
  indexs_.erase(iter);
  updateEvitInfo(key, index_entry, DEL);
  updateStatInfo(index_entry);
  // 异常的数据 后台定时任务进行处理删除合并，清理空间
  return true;
}

template <typename K, typename V> inline bool CacheShard<K, V>::empty() {
  std::shared_lock<std::shared_mutex> lock(shard_mutex_);
  return indexs_.empty();
}

template <typename K, typename V>
inline void CacheShard<K, V>::backendDataMergeTasker() {
  // 后台异步数据清理任务
  // 主要的思路如下：
  // 1. 方式1
  // 遍历索引数据，然后根据索引读取数据，再重新写入到一个新的数据buffer(new_data_)和索引中，最后将data_，indexs_进行替换
  // 方式1中，如果无效数据很多的话，效率会比较高。因为索引的数据比较小
  // 2. 方式2
  // 直接读取data_中的数据，然后解析每个item数据，然后再重新写入到一个新的数据buffer(new_data_)中，最后将data_进行替换，并将索引同步更新
  // 方式2中，如果无效数据很多的话，效率会比较低，因为它需要每个item的数据都进行反序列化，然后才能判断
  // 但方式2不需要遍历索引数据，因此不需要加锁

  std::unordered_map<K, IndexEntry<K>> new_indexs_;
  std::vector<uint8_t> new_data_;
  new_indexs_.reserve(indexs_.size());
  new_data_.reserve(capacity_);
  int new_data_used_size;
  // 下文采用方式1来进行实现
  for (auto &iter : indexs_) {
    auto &key = iter.first;
    auto &index_entry = iter.second;
    if (itemIsExpired(index_entry.index.expire_time)) {
      // 无效数据，直接删除
      // updateStatInfo(index_entry);
      indexs_.erase(key);
      continue;
    }
    // 有效数据
    auto new_index_entry = index_entry;
    new_index_entry.index.start = new_data_.size();
    // 写入数据
    auto start = data_.begin() + index_entry.index.start;
    auto end = start + index_entry.index.length;
    new_data_used_size += index_entry.index.length;
    new_data_.insert(new_data_.end(), start, end);
    // 更新索引
    new_indexs_[key] = new_index_entry;
  }
  // 最后进行替换
  {
    std::unique_lock<std::shared_mutex> lock(shard_mutex_);
    indexs_.swap(new_indexs_);
    data_.swap(new_data_);
    data_used_size_ = new_data_used_size;
    data_invalid_size_.store(0, std::memory_order_relaxed);
    need_evite_.store(false, std::memory_order_relaxed);
  }
}

template <typename K, typename V> inline int64_t CacheShard<K, V>::size() {
  std::shared_lock<std::shared_mutex> lock(shard_mutex_);
  return indexs_.size();
}

template <typename K, typename V> inline void CacheShard<K, V>::clear() {
  std::unique_lock<std::shared_mutex> lock(shard_mutex_);
  indexs_.clear();
  data_.clear();
}

template <typename K, typename V>
inline int64_t CacheShard<K, V>::getSystemCurrentTs() {
  if (parent_) {
    return parent_->getCurrentTs();
  }
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

template <typename K, typename V>
inline bool CacheShard<K, V>::writeIntoData(std::vector<uint8_t> &item_data) {
  //   将item_data的数据移动到data_中
  data_.insert(data_.end(), std::make_move_iterator(item_data.begin()),
               std::make_move_iterator(item_data.end()));
  item_data.clear();
  item_data.shrink_to_fit();
  return true;
}

template <typename K, typename V>
inline void CacheShard<K, V>::serialize(CacheItem<K, V> &item,
                                        std::vector<uint8_t> &data) {
  //   先采用基本类型进行存储
  // todo 后续可以考虑采用varint进行优化
  // 序列化格式
  //   |key_len(8个字节)|value_len(8个字节)|key(key_len个字节)|value(value_len个字节)|expire_time(8个字节)|check_sum(4个字节)|
  //  先写key的长度
  if constexpr (std::is_class_v<K>) {
    Serializer<int64_t>::serialize(item.key.size(), data);
  } else {
    Serializer<int64_t>::serialize(sizeof(item.key), data);
  }
  if constexpr (std::is_class_v<V>) {
    Serializer<int64_t>::serialize(item.val.size(), data);
  } else {
    Serializer<int64_t>::serialize(sizeof(item.val), data);
  }
  // 在写value的长度
  // 再写key的数据
  Serializer<K>::serialize(item.key, data);
  // 在写value的数据
  Serializer<V>::serialize(item.val, data);
  // 再写expire_time
  Serializer<int64_t>::serialize(item.expire_time, data);
  item.check_sum = cal_check_sum(data, 0, data.size());
  //   最后写入校验和
  Serializer<int64_t>::serialize(item.check_sum, data);
}

template <typename K, typename V>
inline bool CacheShard<K, V>::deserialize(std::vector<uint8_t> &data,
                                          CacheItem<K, V> &item) {
  // 序列化格式
  //   |key_len(8个字节)|value_len(8个字节)|key(key_len个字节)|value(value_len个字节)|expire_time(8个字节)|check_sum(4个字节)|
  int64_t key_len, val_len;
  int64_t offset = 0;
  K key;
  V val;
  int64_t expire_time;
  int64_t check_sum;
  if (Serializer<int64_t>::deserialize(data, key_len, offset)) {
    offset += sizeof(int64_t);
  }
  if (Serializer<int64_t>::deserialize(data, val_len, offset)) {
    offset += sizeof(int64_t);
  }
  if (Serializer<K>::deserialize(data, key, offset, key_len)) {
    offset += key_len;
  }
  if (Serializer<V>::deserialize(data, val, offset, val_len)) {
    offset += val_len;
  }
  if (Serializer<int64_t>::deserialize(data, expire_time, offset)) {
    offset += sizeof(int64_t);
  }

  int64_t new_check_sum = cal_check_sum(data, 0, offset);
  if (Serializer<int64_t>::deserialize(data, check_sum, offset)) {
    offset += sizeof(int64_t);
  }
  // 校验校验和
  if (check_sum != new_check_sum) {
    spdlog::error("check_sum is not equal");
    return false;
  }

  // 最后赋值给item
  item.key = key;
  item.val = val;
  item.expire_time = expire_time;
  item.check_sum = check_sum;
  return true;
}

template <typename K, typename V>
inline int64_t CacheShard<K, V>::cal_check_sum(std::vector<uint8_t> &data,
                                               int offset, int length) {
  // 计算校验和
  uLong crc = crc32(0L, Z_NULL, 0);
  return crc32(crc, data.data() + offset, length);
}

} // namespace fast_cache