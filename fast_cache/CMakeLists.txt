cmake_minimum_required(VERSION 3.15)
project(fast_cache VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 C 编译器优化选项为 -O2
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
# 设置 C++ 编译器优化选项为 -O2
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 查找依赖库
find_package(ZLIB REQUIRED)
# find_package(spdlog REQUIRED)  # 如果已安装spdlog


# 添加spdlog（通过FetchContent）
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.11.0
)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(googletest)
FetchContent_MakeAvailable(benchmark)

# 添加主库
add_library(fast_cache INTERFACE)
target_sources(fast_cache INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/fast_cache.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cache_item.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cache_shard.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/serializer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.h

)


# 链接库
target_link_libraries(fast_cache
  INTERFACE
    ZLIB::ZLIB
    spdlog::spdlog
)


# 在文件末尾添加测试目标配置
enable_testing()

# 添加可执行文件（示例）
add_executable(fast_cache_example
  examples/main.cpp
  src/logger.cpp
)

add_executable(cache_shard_test
  test/fast_cache/cache_shard_test.cpp
)

add_executable(fast_cache_benchmark
  benchmark/fast_cache_benchmark.cpp
  src/logger.cpp
)

target_compile_definitions(fast_cache_example
  PRIVATE USE_SPDLOG
)

# 添加头文件搜索路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/spdlog-src/include
)


# 修改可执行文件的包含目录
target_include_directories(fast_cache_example
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(fast_cache_example
  PRIVATE fast_cache
)

target_link_libraries(cache_shard_test
  PRIVATE
    fast_cache
    gtest_main
)

target_link_libraries(fast_cache_benchmark
  PRIVATE
    fast_cache
    benchmark::benchmark
    benchmark::benchmark_main
)

# 安装规则
install(TARGETS fast_cache
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY src/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)

include(GoogleTest)
gtest_discover_tests(cache_shard_test)