#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_

#include <mutex>
#include <unordered_map>
#include "log.h"

template<typename K, typename V>
class ConcurrentMap {
 private:
  std::mutex mapMutex;
  std::unordered_map<K, V> map;
 public:
  ConcurrentMap() : mapMutex(), map() {}

  bool insert(const K &key, const V &value) {
    std::lock_guard guard(mapMutex);
    return map.insert({key, value}).second;
  }

  V& find(const K &key) {
    std::lock_guard guard(mapMutex);
    return map[key];
  }

  bool erase(const K &key) {
    auto iter = map.find(key);
    if (iter != map.end()) {
      std::lock_guard guard(mapMutex);
      map.erase(iter);
      return true;
    } else return false;
  }
};
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_UTILS_CONCURRENTMAP_H_
