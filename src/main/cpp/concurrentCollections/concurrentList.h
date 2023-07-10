#ifndef KOTLIN_TRACER_CONCURRENTLIST_H
#define KOTLIN_TRACER_CONCURRENTLIST_H

#include <shared_mutex>
#include <list>
#include <functional>
#include <mutex>

namespace kotlin_tracer {
template<typename T>
class ConcurrentList {
 private:
  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;
  std::list<T> list_;
  std::shared_mutex list_mutex_;
 public:
  ConcurrentList() : list_(), list_mutex_() {}

  void push_back(const T &value) {
    write_lock guard(list_mutex_);
    list_.push_back(value);
  }

  void push_front(const T &value) {
    write_lock guard(list_mutex_);
    list_.push_front(value);
  }

  bool empty() {
    read_lock guard(list_mutex_);
    return list_.empty();
  }

  T pop_front() {
    bool empty;
    {
      read_lock guard(list_mutex_);
      empty = list_.empty();
    }
    if (empty) {
      return nullptr;
    } else {
      write_lock guard(list_mutex_);
      auto value = list_.front();
      list_.pop_front();
      return value;
    }
  }

  T back() {
    read_lock guard(list_mutex_);
    return list_.back();
  }

  size_t size() {
    read_lock guard(list_mutex_);
    return list_.size();
  }

  void forEach(const std::function<void(T)> &lambda) {
    read_lock guard(list_mutex_);
    for (auto &element : list_) {
      lambda(element);
    }
  }

  T find(const std::function<bool(T)> &lambda) {
    read_lock guard(list_mutex_);
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
