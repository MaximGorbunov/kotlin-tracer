#ifndef KOTLIN_TRACER_CONCURRENTLIST_H
#define KOTLIN_TRACER_CONCURRENTLIST_H

#include <mutex>
#include <list>
#include <functional>

namespace kotlin_tracer {
template<typename T>
class ConcurrentList {
 private:
  std::list<T> list_;
  std::mutex list_mutex_;
 public:
  ConcurrentList() : list_(), list_mutex_() {}

  void push_back(const T &value) {
    std::lock_guard guard(list_mutex_);
    list_.push_back(value);
  }

  void push_front(const T &value) {
    std::lock_guard guard(list_mutex_);
    list_.push_front(value);
  }

  bool empty() {
    return list_.empty();
  }

  T pop_front() {
    std::lock_guard guard(list_mutex_);

    if (list_.empty()) {
      return nullptr;
    } else {
      auto value = list_.front();
      list_.pop_front();
      return value;
    }
  }

  T back() {
    std::lock_guard guard(list_mutex_);
    return list_.back();
  }

  size_t size() {
    return list_.size();
  }

  void forEach(const std::function<void(T)> &lambda) {
    std::lock_guard guard(list_mutex_);
    for (auto &element : list_) {
      lambda(element);
    }
  }

  T find(const std::function<bool(T)> &lambda) {
    std::lock_guard guard(list_mutex_);
    for (auto &element : list_) {
      if (lambda(element)) {
        return element;
      }
    }
    return nullptr;
  }
};
}
#endif //KOTLIN_TRACER_CONCURRENTLIST_H
