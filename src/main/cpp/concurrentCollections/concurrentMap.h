#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_

#include <mutex>
#include <unordered_map>

namespace kotlin_tracer {
template<typename K, typename V>
class ConcurrentMap {
 private:
  std::mutex map_mutex_;
  std::unordered_map<K, V> map_;
 public:
  ConcurrentMap() : map_mutex_(), map_() {}

  bool insert(const K &key, const V &value) {
    std::lock_guard guard(map_mutex_);
    return map_.insert({key, value}).second;
  }

  V &get(const K &key) {
    std::lock_guard guard(map_mutex_);
    return map_[key];
  }

  V &find(const K &key) {
    std::lock_guard guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      return iter->second;
    } else {
      return nullptr;
    }
  }

  V findOrInsert(const K &key, V (creationFunc)()) {
    std::lock_guard guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      return iter->second;
    } else {
      V value = creationFunc();
      map_.insert({key, value});
      return value;
    }
  }

  bool contains(const K &key) {
    std::lock_guard guard(map_mutex_);
    return map_.contains(key);
  }

  bool erase(const K &key) {
    std::lock_guard guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      map_.erase(iter);
      return true;
    } else return false;
  }
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
