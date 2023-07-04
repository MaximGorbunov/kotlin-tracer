#include <dlfcn.h>
#include <memory>
#include <utility>
#include <cstring>
#include <list>
#include <string>

#include "profiler.hpp"
#include "trace/traceTime.hpp"
#include "../utils/log.h"
#include "../utils/suspensionPlot.hpp"

using std::shared_ptr, std::thread, std::string, std::to_string, std::make_unique, std::runtime_error,
    std::make_shared, std::list, std::size;

namespace kotlin_tracer {

static thread_local int64_t currentCoroutineId = NOT_FOUND;

shared_ptr<Profiler> Profiler::instance_;
thread_local jlong Profiler::coroutine_id_;
static std::atomic_long trace_counter(0);

std::shared_ptr<Profiler> Profiler::getInstance() {
  return Profiler::instance_;
}

std::shared_ptr<Profiler> Profiler::create(std::shared_ptr<JVM> jvm,
                                           std::chrono::nanoseconds threshold, const string &output_path) {
  if (instance_ == nullptr) {
    instance_ = shared_ptr<Profiler>(new Profiler(std::move(jvm), threshold, output_path));
  }
  return instance_;
}

void Profiler::signal_action(__attribute__((unused)) int signo,
                             __attribute__((unused)) siginfo_t *siginfo,
                             void *ucontext) {
  if (!active_) return;
  auto trace = std::make_shared<ASGCTCallTrace>(ASGCTCallTrace{});
  trace->env_id = jvm_->getJNIEnv();
  trace->frames = reinterpret_cast<ASGCTCallFrame *>(calloc(30, sizeof(ASGCTCallFrame)));
  trace->num_frames = 0;
  async_trace_ptr_(trace.get(), 30, ucontext);
  if (trace->num_frames > 0) {
    getInstance()->storage_.addRawTrace(currentTimeMs(), trace, pthread_self(), coroutine_id_);
  }
}

Profiler::Profiler(shared_ptr<JVM> jvm, std::chrono::nanoseconds threshold, string output_path)
    : jvm_(std::move(jvm)),
      storage_(),
      method_info_map_(),
      threshold_(threshold),
      interval_(std::chrono::milliseconds(1000)), output_path_(std::move(output_path)) {
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
  active_ = true;
  dlclose(libjvm_handle);
}

void Profiler::startProfiler() {
  profiler_thread_ = std::make_unique<thread>([this] {
    logDebug("[Kotlin-tracer] Profiler thread started");
    jvm_->attachThread();
    std::function<void(shared_ptr<ThreadInfo>)> lambda = [](const shared_ptr<ThreadInfo> &thread) {
      pthread_kill(thread->id, SIGVTALRM);
    };
    while (active_) {
      auto threads = jvm_->getThreads();
      threads->forEach(lambda);
      this->processTraces();
      std::this_thread::sleep_for(interval_);
    }
    jvm_->dettachThread();
    logDebug("end profile thread");
  });
}

void Profiler::stop() {
  logDebug("stop profiler");
  active_ = false;
  profiler_thread_->join();
}

void Profiler::traceStart() {
  auto coroutine_id = currentCoroutineId;
  auto charBuffer = make_unique<char[]>(100);
  pthread_getname_np(pthread_self(), charBuffer.get(), 100);
  auto start = currentTimeNs();
  TraceInfo trace_info{coroutine_id, start, 0};
  storage_.createChildCoroutineStorage(coroutine_id);
  if (storage_.addOngoingTraceInfo(trace_info)) {
    logDebug("trace start: " + to_string(start) + " coroutine:" + to_string(coroutine_id));
  } else {
    throw runtime_error("Found trace that already started");
  }
}

void Profiler::traceEnd(jlong coroutine_id) {
  coroutine_id = coroutine_id == -2 ? currentCoroutineId : coroutine_id;
  auto finish = currentTimeNs();
  auto traceInfo = findOngoingTrace(coroutine_id);
  traceInfo.end = finish;
  removeOngoingTrace(traceInfo.coroutine_id);
  auto elapsedTime = traceInfo.end - traceInfo.start;
  logDebug("trace end: " + to_string(finish) + ":" + to_string(coroutine_id) + " time: "
               + to_string(elapsedTime));
  logDebug("threshold: " + to_string(threshold_.count()));
  if (elapsedTime > threshold_.count()) {
    plot(output_path_ + "/trace" + to_string(++trace_counter) +".html", traceInfo, storage_);
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
    processedRecord->time = rawRecord->time;
    processedRecord->thread_name = thread->name;
    processedRecord->trace_count = rawRecord->trace_count;
    shared_ptr<list<shared_ptr<string>>> methods = make_shared<list<shared_ptr<string>>>();
    for (int i = 0; i < rawRecord->trace_count; i++) {
      ASGCTCallFrame frame = rawRecord->trace->frames[i];
      methods->push_back(processMethodInfo(frame.method_id, frame.line_number));
    }
    processedRecord->method_info = std::move(methods);
    storage_.addProcessedTrace(processedRecord);
    rawRecord = storage_.removeRawTraceHeader();
  }
}

shared_ptr<string> Profiler::processMethodInfo(jmethodID methodId, jint lineno) {
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
    string line = to_string(lineno);
    shared_ptr<string> method = make_shared<string>(className + '.' + name + signature);
    method_info_map_.insert(methodId, method);
    return make_shared<string>(*method + ':' + line);
  } else {
    string line = to_string(lineno);
    return make_shared<string>(*method_info_map_.get(methodId) + ':' + line);
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
  auto parent_id = currentCoroutineId;
  pthread_t current_thread = pthread_self();
  auto thread_info = jvm_->findThread(current_thread);
  logDebug("coroutineCreated tid: " + *thread_info->name + " cid: " + to_string(coroutine_id) +
      " from parentId: " + to_string(currentCoroutineId) + '\n');
  storage_.createCoroutineInfo(coroutine_id);
  if (storage_.containsChildCoroutineStorage(parent_id)) {
    storage_.addChildCoroutine(coroutine_id, parent_id);
    storage_.createChildCoroutineStorage(coroutine_id);
  }
}

void Profiler::coroutineSuspended(jlong coroutine_id) {
  currentCoroutineId = NOT_FOUND;

  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  if (coroutine_info->last_resume > 0) {
    coroutine_info->running_time += (currentTimeNs() - coroutine_info->last_resume);
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
                                                                     make_unique<list<shared_ptr<string>>>()});
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
  currentCoroutineId = coroutine_id;
  coroutine_id_ = coroutine_id;
  storage_.getCoroutineInfo(coroutine_id)->last_resume = currentTimeNs();
  auto suspensionInfo = storage_.getLastSuspensionInfo(coroutine_id);
  if (suspensionInfo != nullptr) {
    suspensionInfo->end = currentTimeNs();
  }
  logDebug("coroutine resumed tid: " + *thread_info->name + ", cid: " + to_string(coroutine_id) + '\n');
}

void Profiler::coroutineCompleted(jlong coroutine_id) {
  currentCoroutineId = NOT_FOUND;
  auto coroutine_info = storage_.getCoroutineInfo(coroutine_id);
  coroutine_info->running_time += (currentTimeNs() - coroutine_info->last_resume);
  logDebug("coroutineCompleted " + to_string(coroutine_id) + '\n');
}
}  // namespace kotlin_tracer
