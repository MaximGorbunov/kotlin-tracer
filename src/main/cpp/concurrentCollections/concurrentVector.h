#ifndef KOTLIN_TRACER_CONCURRENTVECTOR_H
#define KOTLIN_TRACER_CONCURRENTVECTOR_H

#include <shared_mutex>
#include <list>
#include <functional>
#include <mutex>
#include <atomic>

namespace kotlin_tracer {
template<typename T>
class ConcurrentVector {
 private:
  typedef std::shared_lock<std::shared_mutex> read_lock;
  typedef std::unique_lock<std::shared_mutex> write_lock;
  std::vector<T> vector_;
  std::shared_mutex vector_mutex_;
 public:
  ConcurrentVector() : vector_(), vector_mutex_() {}

  void push_back(const T &value) {
    write_lock guard(vector_mutex_);
    vector_.push_back(value);
  }

  T at(size_t id) {
    read_lock guard(vector_mutex_);
    return vector_.at(id);
  }

  size_t size() {
    read_lock guard(vector_mutex_);
    return vector_.size();
  }

  void forEach(const std::function<void(T)> &lambda) {
    read_lock guard(vector_mutex_);
    for (auto &element : vector_) {
      lambda(element);
    }
  }
};
}
#endif //KOTLIN_TRACER_CONCURRENTVECTOR_H
