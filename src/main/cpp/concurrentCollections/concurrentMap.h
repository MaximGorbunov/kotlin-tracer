#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_

#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace kotlin_tracer {
template<typename K, typename V>
class ConcurrentCleanableMap {
 private:

  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;

  std::shared_mutex map_mutex_;
  std::unordered_map<K, V> map_;
  std::vector<K> clean_list_;

 public:
  ConcurrentCleanableMap() : map_mutex_(), map_() {}

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

  bool empty() {
    return map_.empty();
  }

  bool cleanListEmpty() {
    return clean_list_.empty();
  }

  [[maybe_unused]]
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

  void mark_current_values_for_clean() {
    read_lock guard(map_mutex_);
    for (const auto &[key, value] : map_) {
      clean_list_.push_back(key);
    }
  }

  void clean() {
    write_lock guard(map_mutex_);
    while(!clean_list_.empty()) {
      map_.erase(clean_list_.back());
      clean_list_.pop_back();
    }
  }
};
}
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
