#ifndef KOTLIN_TRACER_CONCURRENTLIST_H
#define KOTLIN_TRACER_CONCURRENTLIST_H

#include <mutex>
#include <list>
#include <functional>

namespace kotlinTracer {
template<typename T>
class ConcurrentList {
 private:
  std::list<T> list;
  std::mutex listMutex;
 public:
  ConcurrentList() : list(), listMutex() {}

  void push_back(T value) {
    std::lock_guard guard(listMutex);
    list.push_back(value);
  }

  void push_front(T value) {
    std::lock_guard guard(listMutex);
    list.push_front(value);
  }

  bool empty() {
    return list.empty();
  }

  T pop_front() {
    std::lock_guard guard(listMutex);

    if (list.empty()) {
      return nullptr;
    } else {
      auto value = list.front();
      list.pop_front();
      return value;
    }
  }

  T back() {
    std::lock_guard guard(listMutex);
    return list.back();
  }

  size_t size() {
    return list.size();
  }

  void forEach(std::function<void(T)> &lambda) {
    std::lock_guard guard(listMutex);
    for (auto &element: list) {
      lambda(element);
    }
  }

  T find(std::function<bool(T)> &lambda) {
    std::lock_guard guard(listMutex);
    for (auto &element: list) {
      if (lambda(element)) {
        return element;
      }
    }
    return nullptr;
  }
};
}
#endif //KOTLIN_TRACER_CONCURRENTLIST_H
