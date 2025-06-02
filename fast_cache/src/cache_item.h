#include <cstdint>
namespace fast_cache {
static const int64_t dataNoExpire = -1;
template <typename K, typename V> class CacheItem {
public:
  CacheItem() {}
  CacheItem(const K &key, const V &val, int64_t expire_time=dataNoExpire);
  void set_check_sum(int64_t check_sum);
  int64_t get_check_sum();
  ~CacheItem() {}

public:
  K key;
  V val;
  int64_t expire_time;
  int64_t check_sum;
};


template <typename K, typename V>
CacheItem<K, V>::CacheItem(const K &key, const V &val, int64_t expire_time)
    : key(key), val(val), expire_time(expire_time) {}
template <typename K, typename V>
void CacheItem<K, V>::set_check_sum(int64_t check_sum) {
  this->check_sum = check_sum;
}
template <typename K, typename V> int64_t CacheItem<K, V>::get_check_sum() {
  return this->check_sum;
}
} // namespace fast_cache
