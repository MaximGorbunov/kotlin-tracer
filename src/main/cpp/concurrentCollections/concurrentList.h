#ifndef KOTLIN_TRACER_CONCURRENTLIST_H
#define KOTLIN_TRACER_CONCURRENTLIST_H

#include <mutex>
#include <list>

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

  std::list<T>::iterator begin() {
      return list.begin();
  }

    std::list<T>::iterator end() {
        return list.end();
    }
};
}
#endif //KOTLIN_TRACER_CONCURRENTLIST_H
