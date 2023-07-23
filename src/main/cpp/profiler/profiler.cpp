#include <dlfcn.h>
#include <memory>
#include <utility>
#include <cstring>
#include <list>
#include <string>
#include <sys/resource.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>

#include "profiler.hpp"
#include "trace/traceTime.hpp"
#include "../utils/log.h"
#include "../utils/suspensionPlot.hpp"

using std::shared_ptr, std::unique_ptr, std::thread, std::string, std::to_string, std::make_unique, std::runtime_error,
    std::make_shared, std::list, std::size;

namespace kotlin_tracer {

static thread_local int64_t current_coroutine_id = NOT_FOUND;

shared_ptr<Profiler> Profiler::instance_;
static std::atomic_long trace_counter(0);

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
  trace_coroutine_id.load(std::memory_order_acquire);
  trace_->env_id = jvm_->getJNIEnv();
  async_trace_ptr_(trace_, 30, ucontext);
  logDebug("ticks: " + to_string(trace_->num_frames));
  if (trace_->num_frames <= 0) {
    logDebug("NATIVE FRAME HERE");
    unw_context_t context;
    unw_cursor_t cursor;
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    while (unw_step(&cursor) > 0 && unw_is_signal_frame(&cursor)) {
      //skip current signal frames
    }
    unw_word_t ip, offset;
    char buf[256];
    while(unw_step(&cursor) > 0) {
      unw_get_reg(&cursor, UNW_REG_IP, &ip);
      Dl_info info{};
      if (!unw_get_proc_name(&cursor, buf, sizeof(buf), &offset)) {
//        abi::__cxa_demangle(buf,
        if (dladdr(reinterpret_cast<void*>(ip), &info)) {
          logDebug(string(info.dli_fname) + ":" + string(buf));
        } else {
          logDebug(string(buf));
        }
      } else {
        logDebug("Symbol not found through dladdr");
      };
    };
  }
  int64_t coroutine_id = current_coroutine_id;
  if (current_coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
  }
  trace_coroutine_id.store(coroutine_id, std::memory_order_release);
  trace_coroutine_id.notify_one();
}

Profiler::Profiler(
    shared_ptr<JVM> jvm,
    std::chrono::nanoseconds threshold,
    string output_path,
    std::chrono::nanoseconds interval)
    : jvm_(std::move(jvm)),
      storage_(),
      method_info_map_(),
      threshold_(threshold),
      interval_(interval), output_path_(std::move(output_path)), trace_(nullptr) {
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
  active_.test_and_set(std::memory_order_relaxed);
  profiler_thread_ = std::make_unique<thread>([this] {
    logDebug("[Kotlin-tracer] Profiler thread started");
    jvm_->attachThread();
    std::function<void(shared_ptr<ThreadInfo>)> lambda = [this](const shared_ptr<ThreadInfo> &thread) {
      auto trace = std::make_shared<ASGCTCallTrace>(ASGCTCallTrace{});
      trace->env_id = jvm_->getJNIEnv();
      trace->frames = reinterpret_cast<ASGCTCallFrame *>(calloc(30, sizeof(ASGCTCallFrame)));
      trace->num_frames = 0;
      trace_ = trace.get();
      trace_coroutine_id.store(0, std::memory_order_release);
      pthread_kill(thread->id, SIGVTALRM);
      trace_coroutine_id.wait(0, std::memory_order_acquire);
      if (trace->num_frames > 0) { // Inside JAVA
        getInstance()->storage_.addRawTrace(
            currentTimeMs(),
            trace,
            pthread_self(),
            trace_coroutine_id.load(std::memory_order_relaxed));
      }
    };
    while (active_.test(std::memory_order_relaxed)) {
      auto threads = jvm_->getThreads();
      auto start = currentTimeNs();
      threads->forEach(lambda);
      this->processTraces();
      auto elapsed_time = currentTimeNs() - start;
      auto sleep_time = interval_.count() - elapsed_time;
      if (sleep_time > 0) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_time));
      }
    }
    jvm_->dettachThread();
    logDebug("end profile thread");
  });
  profiler_thread_->detach();
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
  getrusage(RUSAGE_THREAD, &coroutine_info->last_rusage);
  coroutine_info->cpu_system_clock_running_time_us += calculate_elapsed_time(coroutine_info->last_rusage.ru_stime, last_system_time);
  coroutine_info->cpu_user_clock_running_time_us += calculate_elapsed_time(coroutine_info->last_rusage.ru_utime, last_user_time);
  coroutine_info->voluntary_switches += coroutine_info->last_rusage.ru_nvcsw - last_voluntary_context_switch;
  coroutine_info->involuntary_switches += coroutine_info->last_rusage.ru_nivcsw - last_involuntary_context_switch;
}


void Profiler::traceStart() {
  auto coroutine_id = current_coroutine_id;
  if (coroutine_id == -1) { // non suspension function case
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    storage_.createCoroutineInfo(coroutine_id);
  }
  auto charBuffer = make_unique<char[]>(100);
  pthread_getname_np(pthread_self(), charBuffer.get(), 100);
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
  coroutine_id = coroutine_id == -2 ? current_coroutine_id : coroutine_id;
  if (coroutine_id == -1) {
    coroutine_id = static_cast<jlong>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
    calculate_resource_usage(coroutine_info);
  }
  auto finish = currentTimeNs();
  auto traceInfo = findOngoingTrace(coroutine_id);
  traceInfo.end = finish;
  removeOngoingTrace(traceInfo.coroutine_id);
  auto elapsedTime = traceInfo.end - traceInfo.start;
  logDebug("trace end: " + to_string(finish) + ":" + to_string(coroutine_id) + " time: "
               + to_string(elapsedTime));
  logDebug("threshold: " + to_string(threshold_.count()));
  if (elapsedTime > threshold_.count()) {
    plot(output_path_ + "/trace" + to_string(++trace_counter) + ".html", traceInfo, storage_);
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
    shared_ptr<list<unique_ptr<StackFrame>>> methods = make_shared<list<unique_ptr<StackFrame>>>();
    for (int i = 0; i < rawRecord->trace_count; i++) {
      ASGCTCallFrame frame = rawRecord->trace->frames[i];
      methods->push_back(processMethodInfo(frame.method_id, frame.line_number));
    }
    processedRecord->stack_trace = std::move(methods);
    storage_.addProcessedTrace(rawRecord->coroutine_id, processedRecord);
    rawRecord = storage_.removeRawTraceHeader();
  }
}

unique_ptr<StackFrame> Profiler::processMethodInfo(jmethodID methodId, jint lineno) {
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
    jclass klass;
    char *pClassName;
    err = jvmti->GetMethodDeclaringClass(methodId, &klass);
    if (err != JVMTI_ERROR_NONE) {
      logDebug("Error finding class: %d");
    }
    err = jvmti->GetClassSignature(klass, &pClassName, nullptr);
    string className(pClassName, strlen(pClassName) - 1);
    jvmti->Deallocate((unsigned char *) pClassName);
    if (err != JVMTI_ERROR_NONE) {
      logDebug("Error finding class name: " + to_string(err));
    }
    shared_ptr<string> method = make_shared<string>(className + '.' + name + signature);
    method_info_map_.insert(methodId, method);
    return make_unique<StackFrame>(StackFrame{ method, lineno });
  } else {
    string line = to_string(lineno);
    return make_unique<StackFrame>(StackFrame{ method_info_map_.get(methodId), lineno });
  }
}

std::string Profiler::tickToMessage(jint ticks) {
  switch (ticks) {
    case 0 :return "ticks_no_Java_frame";
    case -1 :return "ticks_no_class_load";
    case -2 :return "ticks_GC_active";
    case -3 :return "ticks_unknown_not_Java";
    case -4 :return "ticks_not_walkable_not_Java";
    case -5 :return "ticks_unknown_Java";
    case -6 :return "ticks_not_walkable_Java";
    case -7 :return "ticks_unknown_state";
    case -8 :return "ticks_thread_exit";
    case -9 :return "ticks_deopt";
    case -10 :return "ticks_safepoint";
    default :return "unknown_error";
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
          processMethodInfo(frames[i].method, (jint) frames[i].location));
    }
    storage_.addSuspensionInfo(suspensionInfo);
  }
  logDebug("coroutineSuspend " + to_string(coroutine_id) + '\n');
}

void Profiler::coroutineResumed(jlong coroutine_id) {
  pthread_t current_thread = pthread_self();
  auto thread_info = jvm_->findThread(current_thread);
  current_coroutine_id = coroutine_id;
  storage_.getCoroutineInfo(coroutine_id)->last_resume = currentTimeNs();
  auto suspensionInfo = storage_.getLastSuspensionInfo(coroutine_id);
  if (suspensionInfo != nullptr) {
    suspensionInfo->end = currentTimeNs();
  }
  logDebug("coroutine resumed tid: " + *thread_info->name + ", cid: " + to_string(coroutine_id) + '\n');
}

void Profiler::coroutineCompleted(jlong coroutine_id) {
  current_coroutine_id = NOT_FOUND;
  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
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
