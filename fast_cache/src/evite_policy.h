#pragma once
#include <utility>
namespace fast_cache {
// 数据驱逐策略
enum DataEvitePolicy {
  LRU,
  LFU,
  RANDOM
  // todo ...
};

// LFU的比较器
struct LFUComparer {
  bool operator()(const std::pair<int64_t, int64_t> &a,
                  const std::pair<int64_t, int64_t> &b) const {
    if (a.first == b.first) {
      return a.second < b.second;
    } else {
      return a.first < b.first;
    }
  }
};
// todo 应该抽象一个接口
class EvitePolicyStrategy {
public:
  virtual bool evite_data();
};

} // namespace fast_cache
