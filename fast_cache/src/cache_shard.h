#include "cache_item.h"
#include "serializer.h"
#include <iterator>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>
#include <zlib.h>
namespace fast_cache {

// 存储索引信息
struct CacheIndex {
  int64_t start;
  int64_t length;
};

template <typename K, typename V> class CacheShard {
public:
  CacheShard(int64_t size) { init(size); }
  ~CacheShard() {}
  // 初始化data_的容量
  void init(int64_t size);
  // 插入or 更新数据
  bool set(const K &key, const V &val);
  bool set(const K &key, const V &val, int64_t expire_time);
  bool set(CacheItem<K, V> &item);
  // 获取数据
  bool get(const K &key, V &val);
  bool itemIsValid(fast_cache::CacheItem<K, V> &item);
  // 删除数据
  bool del(const K &key);

  int64_t size();
  bool empty();
  void clear();

private:
  // 写入data_
  bool writeIntoData(std::vector<uint8_t> &data);
  // 序列化
  void serialize(CacheItem<K, V> &item, std::vector<uint8_t> &data);
  // 反序列化
  bool deserialize(std::vector<uint8_t> &data, CacheItem<K, V> &item);

  int64_t cal_check_sum(std::vector<uint8_t> &data, int offset, int length);

  std::unordered_map<K, CacheIndex> indexs_;
  // todo
  // 这里应该采用一个数据容器，比如std::array<uint8_t>。后续应该替换为一个循环队列。
  std::vector<uint8_t> data_;
  int64_t capacity_;
};

template <typename K, typename V>
inline void CacheShard<K, V>::init(int64_t capacity) {
  data_.reserve(capacity);
  capacity_ = capacity;
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
  // 对item进行序列化
  std::vector<uint8_t> item_data;
  serialize(item, item_data);
  // 必须先保存，因为下面会把item_data的数据移动到data_中，所以直接取size()会是0
  // 写成功后再更新索引indexs_
  CacheIndex index;
  index.start = data_.size();
  index.length = item_data.size();;
  // 先写data_
  writeIntoData(item_data);
  // 再记录索引
  indexs_[item.key] = std::move(index);
  return true;
}

template <typename K, typename V>
inline bool CacheShard<K, V>::get(const K &key, V &val) {
  //  先获取索引数据
  auto iter = indexs_.find(key);
  if (iter == indexs_.end()) {
    spdlog::error("key:{} is not found", key);
    return false;
  }
  auto &index = iter->second;
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
  bool suc = deserialize(data, item);
  if (!suc) {
    spdlog::error("deserialize key:{} failed", key);
    return false;
  }
  if (!itemIsValid(item)) {
    spdlog::error("item is expired");
    return false;
  }
  val = item.val;
  //  最后返回数据
  return true;
}
template <typename K, typename V>
inline bool CacheShard<K, V>::itemIsValid(fast_cache::CacheItem<K, V> &item) {
  return item.expire_time == dataNoExpire || item.expire_time > time(NULL);
}
template <typename K, typename V>
inline bool CacheShard<K, V>::del(const K &key) {
  // 从索引数据删除,1表示删除成功，0表示key不存在
  if (indexs_.erase(key) == 0) {
    spdlog::error("key:{} is not found", key);
    return false;
  }
  // 异常的数据 后台定时任务进行处理删除合并，清理空间
  return true;
}

template <typename K, typename V> inline bool CacheShard<K, V>::empty() {
  return indexs_.empty();
}

template <typename K, typename V> inline int64_t CacheShard<K, V>::size() {
  return indexs_.size();
}

template <typename K, typename V> inline void CacheShard<K, V>::clear() {
  indexs_.clear();
  data_.clear();
}

template <typename K, typename V>
inline bool CacheShard<K, V>::writeIntoData(std::vector<uint8_t> &item_data) {
  // 如果数据超过了size_，应该如何处理？
  if (data_.size() + item_data.size() > capacity_) {
    // 没法存储了，先返回
    // todo 后续可以进行 数据淘汰，然后再插入
    //  - LRU、LFU
    spdlog::error("data is full");
    return false;
  }
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
  // 最后校验校验和
  if (check_sum != new_check_sum) {
    spdlog::error("check_sum is not equal");
    return false;
  }
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