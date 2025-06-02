#include <string>
#include <type_traits>
#include <vector>
namespace fast_cache {
template <typename T> struct Serializer {
public:
  static void serialize(const T &obj, std::vector<uint8_t> &data) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type must be trivially copyable for default serialization");
  }
  static bool deserialize(const std::vector<uint8_t> &data, T &obj,
                          int64_t offset, int64_t length = 0) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type must be trivially copyable for default serialization");
  }
};

template <> struct Serializer<int64_t> {
public:
  static void serialize(const int64_t &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(int64_t));
  }

  static bool deserialize(const std::vector<uint8_t> &data, int64_t &obj,
                          int64_t offset, int64_t length = 0) {
    // 在这里实现反序列化逻辑
    if (data.size() < offset + sizeof(int64_t)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const int64_t *>(ptr);
    return true;
  }
};

template <> struct Serializer<int32_t> {
  static void serialize(const int32_t &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(int32_t));
  }

  static bool deserialize(const std::vector<uint8_t> &data, int32_t &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + sizeof(int32_t)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const int32_t *>(ptr);
    return true;
  }
};

template <> struct Serializer<int16_t> {
  static void serialize(const int16_t &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(int16_t));
  }

  static bool deserialize(const std::vector<uint8_t> &data, int16_t &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + sizeof(int16_t)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const int16_t *>(ptr);
    return true;
  }
};

template <> struct Serializer<int8_t> {
  static void serialize(const int8_t &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(int8_t));
  }

  static bool deserialize(const std::vector<uint8_t> &data, int8_t &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + sizeof(int8_t)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const int8_t *>(ptr);
    return true;
  }
};

template <> struct Serializer<float> {
  static void serialize(const float &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(float));
  }

  static bool deserialize(const std::vector<uint8_t> &data, float &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + sizeof(float)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const float *>(ptr);
    return true;
  }
};

template <> struct Serializer<double> {
  static void serialize(const double &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&obj);
    data.insert(data.end(), ptr, ptr + sizeof(double));
  }

  static bool deserialize(const std::vector<uint8_t> &data, double &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + sizeof(double)) {
      return false;
    }
    const uint8_t *ptr = data.data() + offset;
    obj = *reinterpret_cast<const double *>(ptr);
    return true;
  }
};

template <> struct Serializer<std::string> {
  static void serialize(const std::string &obj, std::vector<uint8_t> &data) {
    // 在这里实现序列化逻辑
    const char *ptr = obj.c_str();
    data.insert(data.end(), ptr, ptr + obj.size());
  }

  static bool deserialize(const std::vector<uint8_t> &data, std::string &obj,
                          int64_t offset, int64_t length = 0) {
    if (data.size() < offset + length) {
      return false;
    }
    const char *ptr = reinterpret_cast<const char *>(data.data() + offset);
    obj.assign(ptr, length);
    return true;
  }
};

} // namespace fast_cache