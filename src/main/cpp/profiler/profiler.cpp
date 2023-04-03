#include <dlfcn.h>
#include <memory>

#include "profiler.h"
#include "traceTime.h"
#include "log.h"
#include "jvm.h"

using namespace std;

std::shared_ptr<Profiler> Profiler::instance = shared_ptr<Profiler>(new Profiler());

void Profiler::signal_action(int signo, siginfo_t *siginfo, void *ucontext) {
  auto trace = shared_ptr<ASGCT_CallTrace>((ASGCT_CallTrace *) calloc(1, sizeof(ASGCT_CallTrace)));
  trace->env_id = JVM::getInstance()->getJNIEnv();
  trace->frames = (ASGCT_CallFrame *) calloc(30, sizeof(ASGCT_CallFrame));
  trace->num_frames = 0;
  async_trace_ptr(trace.get(), 30, ucontext);
  log_debug("[Kotlin-tracer] frames count: " + to_string(trace->num_frames));
  if (trace->num_frames > 0) {
    Profiler::getInstance()->storage.addRawTrace(current_time_ms(), trace, pthread_self());
  }
}

Profiler::Profiler() : storage(), methodInfoMap() {
  auto libjvm_handle = dlopen("libjvm.dylib", RTLD_LAZY);
  this->async_trace_ptr = (AsyncGetCallTrace) dlsym(RTLD_DEFAULT, "AsyncGetCallTrace");
  struct sigaction action = {{nullptr}};
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  auto pFunction = [](int signo, siginfo_t *siginfo, void *ucontext) {
    Profiler::getInstance()->signal_action(signo, siginfo, ucontext);
  };
  action.sa_sigaction = pFunction;
  if (sigaction(SIGVTALRM, &action, nullptr) == -1) {
    printf("Error setting handler");
    fflush(stdout);
  }
  active = true;
  dlclose(libjvm_handle);
}

shared_ptr<Profiler> Profiler::getInstance() {
  return instance;
}

void Profiler::start_profiler() {
  log_debug("[Kotlin-tracer] Profiler thread started");
  JVM::getInstance()->attachThread();
  while (active) {
    log_debug("active: " + to_string(active));
    auto threads = JVM::getInstance()->getThreads();
    for (const auto &thread : *threads) {
      pthread_kill(thread->id, SIGVTALRM);
    }
    this->processTraces();
    this_thread::sleep_for(1000ms);
  }
  JVM::getInstance()->dettachThread();
  log_debug("end profile thread");
}

void Profiler::stop() {
  log_debug("stop profiler");
  active = false;
}

bool Profiler::traceStart(const TraceInfo &trace_info) {
  return storage.addOngoingTraceInfo(trace_info);
}

void Profiler::traceEnd(const TraceInfo &trace_info) {
  storage.removeOngoingTraceInfo(trace_info.coroutineId);
  storage.addCompletedTraceInfo(trace_info);
}

void Profiler::removeOngoingTrace(const jlong &coroutineId) {
  storage.removeOngoingTraceInfo(coroutineId);
}

TraceInfo& Profiler::findOngoingTrace(const jlong &coroutineId) {
  return storage.findOngoingTraceInfo(coroutineId);
}

TraceInfo& Profiler::findCompletedTrace(const jlong &coroutineId) {
  return storage.findCompletedTraceInfo(coroutineId);
}

void Profiler::processTraces() {
  auto rawRecord = storage.removeRawTraceHeader();
  while (rawRecord != nullptr) {
    auto processedRecord =
        shared_ptr<ProcessedTraceRecord>((ProcessedTraceRecord *) calloc(1, sizeof(ProcessedTraceRecord)));
    auto thread = JVM::getInstance()->findThread(rawRecord->thread);
    if (thread == nullptr) log_info("Cannot find thread: " + *thread->name);
    processedRecord->time = rawRecord->time;
    processedRecord->thread_name = thread->name;
    processedRecord->trace_count = rawRecord->trace_count;
    shared_ptr<list<shared_ptr<string>>> methods = make_shared<list<shared_ptr<string>>>();
    for (int i = 0; i < rawRecord->trace_count; i++) {
      ASGCT_CallFrame frame = rawRecord->trace->frames[i];
      methods->push_back(processMethodInfo(frame.method_id, frame.lineno));
    }
    processedRecord->methodInfo = std::move(methods);
    storage.addProcessedTrace(processedRecord);
    rawRecord = storage.removeRawTraceHeader();
  }

}

shared_ptr<string> Profiler::processMethodInfo(jmethodID methodId, jint lineno) {
  auto methodIterator = methodInfoMap.find(methodId);
  if (methodIterator == methodInfoMap.end()) {
    auto jvmti = JVM::getInstance()->getJvmTi();
    char *pName;
    char *pSignature;
    char *pGeneric;

    jvmtiError err = jvmti->GetMethodName(methodId, &pName, &pSignature, &pGeneric);
    if (err != JVMTI_ERROR_NONE) {
      printf("Error finding method info: %d\n", err);
    }
    string name(pName);
    string signature(pSignature);
    string generic;
    if (pGeneric != nullptr) {
      generic = string(pGeneric);
    }
    jvmti->Deallocate((unsigned char *) pName);
    jvmti->Deallocate((unsigned char *) pSignature);
    jvmti->Deallocate((unsigned char *) pGeneric);
    jclass klass;
    char *pClassName;
    err = jvmti->GetMethodDeclaringClass(methodId, &klass);
    if (err != JVMTI_ERROR_NONE) {
      log_debug("Error finding class: %d");
    }
    err = jvmti->GetClassSignature(klass, &pClassName, nullptr);
    string className(pClassName, strlen(pClassName) - 1);
    jvmti->Deallocate((unsigned char *) pClassName);
    if (err != JVMTI_ERROR_NONE) {
      log_debug("Error finding class name: " + to_string(err));
    }
    string line = to_string(lineno);
    const shared_ptr<string> &method = make_shared<string>(className + '.' + name + signature + generic);
    methodInfoMap[methodId] = method;
    return make_shared<string>(*method + ':' + line);
  } else {
    string line = to_string(lineno);
    return make_shared<string>(*methodIterator->second + ':' + line);
  }
}
