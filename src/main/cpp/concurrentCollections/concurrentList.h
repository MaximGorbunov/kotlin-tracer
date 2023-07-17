#ifndef KOTLIN_TRACER_CONCURRENTLIST_H
#define KOTLIN_TRACER_CONCURRENTLIST_H

#include <shared_mutex>
#include <list>
#include <functional>
#include <mutex>
#include <atomic>

namespace kotlin_tracer {
template<typename T>
class ConcurrentList {
 private:
  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;
  std::list<T> list_;
  std::shared_mutex list_mutex_;
  std::atomic_int clean_until_;
 public:
  ConcurrentList() : list_(), list_mutex_(), clean_until_(0) {}

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

  void pop_back() {
    bool empty;
    {
      read_lock guard(list_mutex_);
      empty = list_.empty();
    }
    if (!empty) {
      write_lock guard(list_mutex_);
      list_.pop_back();
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

  void forEachFiltered(const std::function<bool(T)> &filter, const std::function<void(T)> &lambda) {
    read_lock guard(list_mutex_);
    for (auto &element : list_) {
      if (filter(element)) {
        lambda(element);
      }
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

  void mark_for_clean() {
    read_lock guard(list_mutex_);
    clean_until_.store(list_.size(), std::memory_order_relaxed);
  }

  void clean() {
    write_lock guard(list_mutex_);
    auto until = clean_until_.load(std::memory_order_relaxed);
    for (int i = 0; i < until; ++i) {
      list_.pop_front();
    }
  }
};
}
#endif //KOTLIN_TRACER_CONCURRENTLIST_H
