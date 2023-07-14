#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_

#include <shared_mutex>
#include <unordered_map>

namespace kotlin_tracer {
template<typename K, typename V>
class ConcurrentCleanableMap {
 private:
  struct MarkValue {
    std::atomic_flag clean;
    V value;
    MarkValue(const V &value) : value(value) {};
  };

  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;

  std::shared_mutex map_mutex_;
  std::unordered_map<K, std::unique_ptr<MarkValue>> map_;
 public:
  ConcurrentCleanableMap() : map_mutex_(), map_() {}

  bool insert(const K &key, const V &value) {
    write_lock guard(map_mutex_);
    return map_.insert({key, std::make_unique<MarkValue>(value)}).second;
  }

  V &get(const K &key) {
    read_lock guard(map_mutex_);
    return map_[key]->value;
  }

  V &at(const K &key) {
    read_lock guard(map_mutex_);
    return map_.at(key)->value;
  }

  [[maybe_unused]]
  V &find(const K &key) {
    read_lock guard(map_mutex_);
    auto iter = map_.find(key);
    if (iter != map_.end()) {
      return iter->second->value;
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

  void mark_current_values_for_clean() {
    read_lock guard(map_mutex_);
    for (const auto &[key, value] : map_) {
      value->clean.test_and_set(std::memory_order_relaxed);
    }
  }

  void clean() {
    write_lock guard(map_mutex_);
    for (const auto &[key, value]: map_) {
      if (value->clean.test(std::memory_order_relaxed))  {
        map_.erase(key);
      }
    }
  }
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
