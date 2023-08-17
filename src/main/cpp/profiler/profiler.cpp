#include "profiler.hpp"

#include <dlfcn.h>
#include <sys/resource.h>
#include <memory>
#include <utility>
#include <cstring>
#include <list>
#include <string>

#include "unwind/apple_aarch64.inline.hpp"
#include "unwind/apple_x86_64.inline.hpp"
#include "unwind/linux_x86_64.inline.hpp"
#include "trace/traceTime.hpp"
#include "../utils/log.h"
#include "../utils/suspensionPlot.hpp"

#ifdef __APPLE__
#define RUSAGE_KIND RUSAGE_SELF
#elif defined(__linux__)
#define RUSAGE_KIND RUSAGE_THREAD
#endif

using std::shared_ptr, std::unique_ptr, std::thread, std::string, std::to_string, std::make_unique, std::runtime_error,
    std::make_shared, std::list, std::size;

namespace kotlin_tracer {

static thread_local int64_t current_coroutine_id = NOT_FOUND;

shared_ptr<Profiler> Profiler::instance_;

std::shared_ptr<Profiler> Profiler::getInstance() {
  return Profiler::instance_;
}

std::shared_ptr<Profiler> Profiler::create(std::shared_ptr<JVM> jvm,
                                           std::chrono::nanoseconds threshold,
                                           const string &output_path,
                                           const std::chrono::nanoseconds interval) {
  if (instance_ == nullptr) {
    instance_ = shared_ptr<Profiler>(new Profiler(std::move(jvm), threshold, output_path, interval));
  }
  return instance_;
}

void Profiler::signal_action(__attribute__((unused)) int signo,
                             __attribute__((unused)) siginfo_t *siginfo,
                             void *ucontext) {
  if (!active_.test(std::memory_order_relaxed)) return;
  auto id = trace_counter.fetch_add(1, std::memory_order_acquire);
  auto trace = traces_->at(id);
  unwind_stack(static_cast<ucontext_t *>(ucontext), jvm_, trace.get());
  int64_t coroutine_id = current_coroutine_id;
  if (current_coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  trace->coroutine_id = coroutine_id;
  trace->ready.test_and_set(std::memory_order::release);
  trace->ready.notify_one();
}

Profiler::Profiler(
    shared_ptr<JVM> jvm,
    std::chrono::nanoseconds threshold,
    string output_path,
    std::chrono::nanoseconds interval)
    : jvm_(std::move(jvm)),
      storage_(),
      method_info_map_(),
      class_info_map_(),
      threshold_(threshold),
      interval_(interval), output_path_(std::move(output_path)), traces_(nullptr) {
  auto libjvm_handle = dlopen("libjvm.so", RTLD_LAZY);
  this->async_trace_ptr_ = (AsyncGetCallTrace) dlsym(RTLD_DEFAULT, "AsyncGetCallTrace");
  struct sigaction action{};
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  auto pFunction = [](int signo, siginfo_t *siginfo, void *ucontext) {
    getInstance()->signal_action(signo, siginfo, ucontext);
  };
  action.sa_sigaction = pFunction;
  if (sigaction(SIGVTALRM, &action, nullptr) == -1) {
    printf("Error setting handler");
    fflush(stdout);
  }
  dlclose(libjvm_handle);
}

void Profiler::startProfiler() {
  if (!active_.test_and_set(std::memory_order_relaxed)) {
    profiler_thread_ = std::make_unique<thread>([this] {
      logDebug("[Kotlin-tracer] Profiler thread started");
      jvm_->attachThread();
      std::function<void(shared_ptr<ThreadInfo>)> lambda = [this](const shared_ptr<ThreadInfo> &thread) {
        if (thread->current_traces.load(std::memory_order::relaxed) > 0) {
          auto array = make_unique<InstructionInfo[]>(30);
          auto trace = std::make_shared<AsyncTrace>(std::move(array), 30, thread->id);
          traces_->push_back(trace);
          pthread_kill(thread->id, SIGVTALRM);
        }
      };
      while (active_.test(std::memory_order_relaxed)) {
        auto threads = jvm_->getThreads();
        auto start = currentTimeNs();
        traces_ = std::make_unique<ConcurrentVector<shared_ptr<AsyncTrace>>>();
        trace_counter.store(0, std::memory_order_release);
        threads->forEach(lambda);
        std::function<void(shared_ptr<AsyncTrace>)> read_traces = [](const shared_ptr<AsyncTrace> &trace) {
          if (trace != nullptr) {
            trace->ready.wait(false, std::memory_order_acquire);
            getInstance()->storage_.addRawTrace(
                currentTimeNs(),
                trace,
                trace->thread_id,
                trace->coroutine_id);
          }
        };
        traces_->forEach(read_traces);

        this->processTraces();
        auto elapsed_time = currentTimeNs() - start;
        auto sleep_time = interval_.count() - elapsed_time;
        if (sleep_time > 0) {
          std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_time));
        } else {
          logDebug("sleep time:" + to_string(sleep_time));
        }
      }
      jvm_->dettachThread();
      logDebug("end profile thread");
    });
    profiler_thread_->detach();
  }
}

void Profiler::stop() {
  logDebug("stop profiler");
  active_.clear(std::memory_order_relaxed);
  profiler_thread_.reset();
  logDebug("stop profiler finished");
}

static inline TraceTime calculate_elapsed_time(const timeval &end, const timeval &begin) {
  std::chrono::seconds seconds(end.tv_sec - begin.tv_sec);
  std::chrono::microseconds microseconds(end.tv_usec - begin.tv_usec);
  return std::chrono::duration_cast<std::chrono::microseconds>(seconds).count() + microseconds.count();
}

static inline void calculate_resource_usage(const shared_ptr<TraceStorage::CoroutineInfo> &coroutine_info) {
  coroutine_info->wall_clock_running_time += (currentTimeNs() - coroutine_info->last_resume);
  auto last_system_time = coroutine_info->last_rusage.ru_stime;
  auto last_user_time = coroutine_info->last_rusage.ru_utime;
  auto last_voluntary_context_switch = coroutine_info->last_rusage.ru_nvcsw;
  auto last_involuntary_context_switch = coroutine_info->last_rusage.ru_nivcsw;
  getrusage(RUSAGE_KIND, &coroutine_info->last_rusage);
  coroutine_info->cpu_system_clock_running_time_us +=
      calculate_elapsed_time(coroutine_info->last_rusage.ru_stime, last_system_time);
  coroutine_info->cpu_user_clock_running_time_us +=
      calculate_elapsed_time(coroutine_info->last_rusage.ru_utime, last_user_time);
  coroutine_info->voluntary_switches += coroutine_info->last_rusage.ru_nvcsw - last_voluntary_context_switch;
  coroutine_info->involuntary_switches += coroutine_info->last_rusage.ru_nivcsw - last_involuntary_context_switch;
}

void Profiler::traceStart(jlong coroutine_id) {
  logDebug("coroutine:" + to_string(coroutine_id));
  if (coroutine_id == -2) {  // non suspension function case
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    storage_.createCoroutineInfo(coroutine_id);
  }
  jvm_->findThread(pthread_self())->current_traces.fetch_add(1, std::memory_order_relaxed);
  auto start = currentTimeNs();
  TraceInfo trace_info{coroutine_id, start, 0};
  storage_.createChildCoroutineStorage(coroutine_id);
  if (storage_.addOngoingTraceInfo(trace_info)) {
    logDebug("trace start: " + to_string(start) + " coroutine:" + to_string(coroutine_id));
  } else {
    throw runtime_error("Found trace that already started: " + to_string(coroutine_id));
  }
}

void Profiler::traceEnd(jlong coroutine_id) {
  jvm_->findThread(pthread_self())->current_traces.fetch_sub(1, std::memory_order_relaxed);
  if (coroutine_id == -2) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  if (coroutine_info == nullptr) {
    logDebug("Found nullptr for c: " + to_string(coroutine_id));
  }
  calculate_resource_usage(coroutine_info);
  auto finish = currentTimeNs();
  auto traceInfo = findOngoingTrace(coroutine_id);
  traceInfo.end = finish;
  removeOngoingTrace(traceInfo.coroutine_id);
  auto elapsedTime = traceInfo.end - traceInfo.start;
  logDebug("trace end: " + to_string(finish) + ":" + to_string(coroutine_id) + " time: "
               + to_string(elapsedTime));
  logDebug("threshold: " + to_string(threshold_.count()));
  if (elapsedTime > threshold_.count()) {
    plot(output_path_ + "/trace" + to_string(output_counter.fetch_add(1, std::memory_order_relaxed)) + ".html",
         traceInfo,
         storage_);
  }
}

void Profiler::removeOngoingTrace(const jlong &coroutine_id) {
  storage_.removeOngoingTraceInfo(coroutine_id);
}

TraceInfo &Profiler::findOngoingTrace(const jlong &t_coroutine_id) {
  return storage_.findOngoingTraceInfo(t_coroutine_id);
}

void Profiler::processTraces() {
  auto rawRecord = storage_.removeRawTraceHeader();
  while (rawRecord != nullptr) {
    auto processedRecord = make_shared<ProcessedTraceRecord>(ProcessedTraceRecord{});
    auto thread = jvm_->findThread(rawRecord->thread);
    if (thread == nullptr) logInfo("Cannot get thread: " + *thread->name);
    processedRecord->thread_name = thread->name;
    processedRecord->time = rawRecord->time;
    shared_ptr<list<unique_ptr<StackFrame>>> methods = make_shared<list<unique_ptr<StackFrame>>>();
    for (int i = 0; i < rawRecord->trace_count; i++) {
      auto instruction = rawRecord->trace->instructions[i];
      methods->push_back(processProfilerMethodInfo(instruction));
    }
    processedRecord->stack_trace = std::move(methods);
    storage_.addProcessedTrace(rawRecord->coroutine_id, processedRecord);
    rawRecord = storage_.removeRawTraceHeader();
  }
}

unique_ptr<StackFrame> Profiler::processMethodInfo(jmethodID methodId) {
  if (!method_info_map_.contains(methodId)) {
    auto jvmti = jvm_->getJvmTi();
    char *pName;
    char *pSignature;
    char *pGeneric;

    jvmtiError err = jvmti->GetMethodName(methodId, &pName, &pSignature, &pGeneric);
    if (err != JVMTI_ERROR_NONE) {
      printf("Error finding method info: %d\n", err);
    }
    string name(pName);
    string signature(pSignature);
    jvmti->Deallocate((unsigned char *) pName);
    jvmti->Deallocate((unsigned char *) pSignature);
    jvmti->Deallocate((unsigned char *) pGeneric);
    if (err != JVMTI_ERROR_NONE) {
      logDebug("Error finding class: %d");
    }
    shared_ptr<string> className;
    if (class_info_map_.contains(methodId)) {
      className = class_info_map_.get(methodId);
    } else {
      jclass klass;
      err = jvmti->GetMethodDeclaringClass(methodId, &klass);
      if (err != JVMTI_ERROR_NONE) {
        logError("Failed to get class info");
      }
      char *pClassName;
      err = jvmti->GetClassSignature(klass, &pClassName, nullptr);
      className = make_shared<string>(pClassName, strlen(pClassName) - 1);
      class_info_map_.insert(methodId, className);
      jvmti->Deallocate((unsigned char *) pClassName);
    }
    if (err != JVMTI_ERROR_NONE) {
      logDebug("Error finding class name: " + to_string(err));
    }
    shared_ptr<string> method = make_shared<string>(name + signature);
    method_info_map_.insert(methodId, method);
    return make_unique<StackFrame>(StackFrame{method.get(), className.get()});
  } else {
    return make_unique<StackFrame>(StackFrame{method_info_map_.get(methodId).get(),
                                              class_info_map_.get(methodId).get()});
  }
}

unique_ptr<StackFrame> Profiler::processProfilerMethodInfo(const InstructionInfo &instruction_info) {
  if (instruction_info.java_frame) {
    auto jmethodId = reinterpret_cast<jmethodID>(instruction_info.instruction);
    if (jmethodId != nullptr) {
      if (!method_info_map_.contains(jmethodId)) {
        auto jvmti = jvm_->getJvmTi();
        char *pName;
        char *pSignature;
        char *pGeneric;

        jvmtiError err = jvmti->GetMethodName(jmethodId, &pName, &pSignature, &pGeneric);
        if (err != JVMTI_ERROR_NONE) {
          printf("Error finding profiler method info: %d\n", err);
        }
        string name(pName);
        string signature(pSignature);
        jvmti->Deallocate((unsigned char *) pName);
        jvmti->Deallocate((unsigned char *) pSignature);
        jvmti->Deallocate((unsigned char *) pGeneric);
        shared_ptr<string> className;
        if (class_info_map_.contains(jmethodId)) {
          className = class_info_map_.get(jmethodId);
        } else {
          jclass klass;
          err = jvmti->GetMethodDeclaringClass(jmethodId, &klass);
          if (err != JVMTI_ERROR_NONE) {
            logError("Failed to get class info");
          }
          char *pClassName;
          err = jvmti->GetClassSignature(klass, &pClassName, nullptr);
          className = make_shared<string>(pClassName, strlen(pClassName) - 1);
          class_info_map_.insert(jmethodId, className);
          jvmti->Deallocate((unsigned char *) pClassName);
        }
        if (err != JVMTI_ERROR_NONE) {
          logDebug("Error finding class name: " + to_string(err));
        }
        shared_ptr<string> method = make_shared<string>(name + signature);
        method_info_map_.insert(jmethodId, method);
        return make_unique<StackFrame>(StackFrame{method.get(), className.get()});
      } else {
        return make_unique<StackFrame>(StackFrame{method_info_map_.get(jmethodId).get(),
                                                  class_info_map_.get(jmethodId).get()});
      }
    } else {
      return make_unique<StackFrame>(StackFrame{&unknown_java, &empty});
    }
  } else {
    const auto function_info = jvm_->getNativeFunctionInfo(instruction_info.instruction);
    if (function_info != nullptr) {
      return make_unique<StackFrame>(StackFrame{const_cast<string *>(&function_info->name),
                                                function_info->lib_name.get()});
    } else {
      return make_unique<StackFrame>(StackFrame{&unknown_native, &empty});
    }
  }
}

void Profiler::coroutineCreated(jlong coroutine_id) {
  auto parent_id = current_coroutine_id;
  pthread_t current_thread = pthread_self();
  auto thread_info = jvm_->findThread(current_thread);
  logDebug("coroutineCreated tid: " + *thread_info->name + " cid: " + to_string(coroutine_id) +
      " from parentId: " + to_string(current_coroutine_id) + '\n');
  storage_.createCoroutineInfo(coroutine_id);
  if (storage_.containsChildCoroutineStorage(parent_id)) {
    storage_.addChildCoroutine(coroutine_id, parent_id);
    storage_.createChildCoroutineStorage(coroutine_id);
  }
}

void Profiler::coroutineSuspended(jlong coroutine_id) {
  current_coroutine_id = NOT_FOUND;

  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  if (coroutine_info == nullptr) return;
  if (coroutine_info->last_resume > 0) {
    calculate_resource_usage(coroutine_info);
    coroutine_info->last_resume = 0;  // To not count multiple times chain
  }
  ::jthread thread;
  jvmtiFrameInfo frames[20];
  jint framesCount;
  auto jvmti = jvm_->getJvmTi();
  jvmti->GetCurrentThread(&thread);
  jvmtiError err = jvmti->GetStackTrace(thread, 0, size(frames), frames, &framesCount);
  if (err == JVMTI_ERROR_NONE && framesCount >= 1) {
    auto suspensionInfo = make_shared<SuspensionInfo>(SuspensionInfo{coroutine_id, currentTimeNs(), 0,
                                                                     make_unique<list<std::unique_ptr<StackFrame>>>()});
    for (int i = 0; i < framesCount; ++i) {
      suspensionInfo->suspension_stack_trace->push_back(
          processMethodInfo(frames[i].method));
    }
    storage_.addSuspensionInfo(suspensionInfo);
  }
  logDebug("coroutineSuspend " + to_string(coroutine_id) + '\n');
}

void Profiler::coroutineResumed(jlong coroutine_id) {
  pthread_t current_thread = pthread_self();
  auto thread_info = jvm_->findThread(current_thread);
  current_coroutine_id = coroutine_id;
  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  if (coroutine_info == nullptr) return;
  coroutine_info->last_resume = currentTimeNs();
  auto suspensionInfo = storage_.getLastSuspensionInfo(coroutine_id);
  if (suspensionInfo != nullptr) {
    suspensionInfo->end = currentTimeNs();
  }
  logDebug("coroutine resumed tid: " + *thread_info->name + ", cid: " + to_string(coroutine_id) + '\n');
}

void Profiler::coroutineCompleted(jlong coroutine_id) {
  current_coroutine_id = NOT_FOUND;
  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  if (coroutine_info == nullptr) return;
  calculate_resource_usage(coroutine_info);
  logDebug("coroutineCompleted " + to_string(coroutine_id) + '\n');
}

void Profiler::gcStart() {
  storage_.gcStart();
}

void Profiler::gcFinish() {
  storage_.gcFinish();
}
}  // namespace kotlin_tracer
