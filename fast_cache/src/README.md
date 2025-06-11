# Fast Cache

## 简介
采用c++实现的高性能单机缓存组件。

## 功能特性
支持高并发读写、线程安全、超时时间、数据淘汰策略
## 同类组件对比
| 组件名 | GitHub/官方地址 | Stars | 核心功能 | 优点 | 缺点 |
|-------|----------------|-------|---------|------|------|
| Cachelib | [GitHub](https://github.com/facebook/CacheLib) | ⭐3.4k | 内存缓存、多级淘汰策略、原子操作 | 生产级质量，支持LRU/LFU/FIFO | 配置复杂 |
| Caffeine C++ | [GitHub](https://github.com/actor-framework/caffeine) | ⭐1.2k | 近实时缓存、自动负载均衡 | 高命中率算法，低延迟 | 生态不完善 |
| TBB concurrent_hash_map | [官网](https://software.intel.com/content/www/us/en/develop/documentation/tbb-documentation/top.html) | - | 并发哈希表、原子操作 | 线程安全，高性能 | 需二次开发 |
| Poco::LRUCache | [官网](https://pocoproject.org/) | - | LRU缓存、TTL支持 | 开箱即用，文档齐全 | 性能中等 |
| LruCache | [GitHub](https://github.com/lamerman/cpp-lru-cache) | ⭐300 | 基础LRU、线程安全 | 轻量级，单头文件 | 功能单一 |

核心功能说明：
1. Cachelib：支持内存分配策略和多种淘汰算法组合
2. Caffeine：采用现代缓存算法，自动优化工作集
3. TBB：提供基础并发数据结构
4. Poco：完整的LRU实现，含过期机制
5. LruCache：最小化实现的线程安全LRU
## 编译安装
```shell
 cd fast_cache
 mkdir build
 cd build
 # 生成clangd需要的构建索引的json文件，compile_commands.json
 cmake  -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
 make
```
## 整体设计思路

### 1. 整体架构
{补充框架图}
### 2. 接口设计
整个cache组件包括如下几个接口：
#### 2.1 init()初始化
#### 2.2 get()获取数据
#### 2.3 set()写入数据
#### 2.4 del()删除数据
#### 2.5 缓存统计信息
#### 2.6 持久化
### 3. 数据结构
FastCache
- 可以指定整个缓存的内存大小
- 定义256个哈希槽，然后每个槽对应一个缓存结构_cache(CacheShard)，对每个槽进行加锁
    - 每个缓存结构CacheShard包含索引和数据两部分
        - 索引部分index：首先根据key从索引结构中读取索引，如果不存在，则直接返回，存在后对索引进行解析
            - 索引数据结构：采用map结构map<key,value>,key为原始的key，value为索引数据
            - value索引数据：当前的数据写入data中的起始位置，数据的大小
        - 数据部分data：根据索引数据(start、length)从数据结构中读取数据，然后解析数据，解析成功后返回数据
        -- 数据item的格式：key_length(4个字节):value_length(8个字节)+key(key_length个字节)+value(value_length个字节)+expire_time(8个字节)+校验和(4个字节)
### 4. 如何使用

### 5. 性能测试

## 待实现功能
0. ~~功能的正确性验证 ~~
1. 后台异步数据的清理  
2. <del>LRU/LFU cache的实现</del>
3. varint编码
4. 数据压缩
5. 数据的持久化
