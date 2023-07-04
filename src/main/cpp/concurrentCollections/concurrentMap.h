#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_

#include <shared_mutex>
#include <unordered_map>

namespace kotlin_tracer {
template<typename K, typename V>
class ConcurrentMap {
 private:
  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;
  std::shared_mutex map_mutex_;
  std::unordered_map<K, V> map_;
 public:
  ConcurrentMap() : map_mutex_(), map_() {}

  bool insert(const K &key, const V &value) {
    write_lock guard(map_mutex_);
    return map_.insert({key, value}).second;
  }

  V &get(const K &key) {
    read_lock guard(map_mutex_);
    return map_[key];
  }

  V &at(const K &key) {
    read_lock guard(map_mutex_);
    return map_.at(key);
  }

  V &find(const K &key) {
    read_lock guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      return iter->second;
    } else {
      return nullptr;
    }
  }

  bool contains(const K &key) {
    read_lock guard(map_mutex_);
    return map_.contains(key);
  }

  bool erase(const K &key) {
    write_lock guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      map_.erase(iter);
      return true;
    } else return false;
  }
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
