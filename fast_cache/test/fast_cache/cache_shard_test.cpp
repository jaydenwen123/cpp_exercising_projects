#include "cache_shard.h"
#include <gtest/gtest.h>
#include <string>

// 假设 CacheShard 是一个类，我们需要测试它的基本功能
class CacheShardTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 初始化代码，如果有需要
  }

  void TearDown() override {
    // 清理代码，如果有需要
  }
};

// 测试 CacheShard 的构造函数
TEST_F(CacheShardTest, Constructor) {
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);
  // 验证构造后的初始状态
  EXPECT_EQ(cache.size(), 0);
  EXPECT_TRUE(cache.empty());
}

// 测试插入和查找功能
TEST_F(CacheShardTest, SetAndGet) {
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);
  std::string key = "test_key";
  std::string value = "test_value";

  // 插入键值对
  cache.set(key, value);

  // 查找键值对
  std::string val;
  auto found = cache.get(key,val);
  EXPECT_TRUE(found);
  EXPECT_EQ(value, val);
}

// 测试删除功能
TEST_F(CacheShardTest, SetAndDel) {
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);
  std::string key = "test_key";
  std::string value = "test_value";

  cache.set(key, value);
  EXPECT_EQ(cache.size(), 1);

  // 删除键值对
  cache.del(key);
  EXPECT_EQ(cache.size(), 0);
  std::string get_val;
  EXPECT_FALSE(cache.get(key,get_val));
}

// 测试大小和空状态
TEST_F(CacheShardTest, SizeAndEmpty) {
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);
  EXPECT_TRUE(cache.empty());

  cache.set("key1", "value1");
  EXPECT_EQ(cache.size(), 1);
  EXPECT_FALSE(cache.empty());

  cache.set("key2", "value2");
  EXPECT_EQ(cache.size(), 2);
}

// 测试清除所有内容
TEST_F(CacheShardTest, Clear) {
  fast_cache::CacheShard<std::string, std::string> cache(100 * 1024 * 1024);
  cache.set("key1", "value1");
  cache.set("key2", "value2");

  EXPECT_EQ(cache.size(), 2);
  cache.clear();
  EXPECT_EQ(cache.size(), 0);
  EXPECT_TRUE(cache.empty());
}