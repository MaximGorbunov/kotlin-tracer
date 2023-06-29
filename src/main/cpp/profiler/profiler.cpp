#include <dlfcn.h>
#include <memory>
#include <utility>
#include <cstring>

#include "profiler.hpp"
#include "trace/traceTime.hpp"
#include "trace/coroutineTrace.hpp"
#include "../utils/log.h"

using namespace std;

shared_ptr<kotlinTracer::Profiler> kotlinTracer::Profiler::instance;
thread_local jlong kotlinTracer::Profiler::m_coroutineId;

std::shared_ptr<kotlinTracer::Profiler> kotlinTracer::Profiler::getInstance() {
  return kotlinTracer::Profiler::instance;
}

std::shared_ptr<kotlinTracer::Profiler> kotlinTracer::Profiler::create(std::shared_ptr<kotlinTracer::JVM> t_jvm,
                                                                       std::chrono::nanoseconds t_threshold) {
  if (instance == nullptr) {
    instance = shared_ptr<kotlinTracer::Profiler>(new kotlinTracer::Profiler(std::move(t_jvm), t_threshold));
  }
  return instance;
}

void kotlinTracer::Profiler::signal_action(int t_signo, siginfo_t *t_siginfo, void *t_ucontext) {
  if (!m_active) return;
  auto trace = std::make_shared<ASGCTCallTrace>(ASGCTCallTrace{});
  trace->envId = m_jvm->getJNIEnv();
  trace->frames = (ASGCTCallFrame *) calloc(30, sizeof(ASGCTCallFrame));
  trace->numFrames = 0;
  m_asyncTracePtr(trace.get(), 30, t_ucontext);
  if (trace->numFrames < 0) {
    logDebug("[Kotlin-tracer] frames not found: " + tickToMessage(trace->numFrames));
  }
  if (trace->numFrames > 0) {
    getInstance()->m_storage.addRawTrace(kotlinTracer::currentTimeMs(), trace, pthread_self(), m_coroutineId);
  }
}

kotlinTracer::Profiler::Profiler(shared_ptr<kotlinTracer::JVM> t_jvm, std::chrono::nanoseconds t_threshold)
    : m_jvm(std::move(t_jvm)), m_storage(), m_methodInfoMap(), m_threshold(t_threshold) {
  auto libjvm_handle = dlopen("libjvm.so", RTLD_LAZY);
  this->m_asyncTracePtr = (AsyncGetCallTrace) dlsym(RTLD_DEFAULT, "AsyncGetCallTrace");
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
  m_active = true;
  dlclose(libjvm_handle);
}

void kotlinTracer::Profiler::startProfiler() {
  m_profilerThread = std::make_unique<thread>([this] {
    logDebug("[Kotlin-tracer] Profiler thread started");
    m_jvm->attachThread();
    std::function<void(shared_ptr<ThreadInfo>)> lambda = [](const shared_ptr<ThreadInfo> &thread) {
      pthread_kill(thread->id, SIGVTALRM);
    };
    while (m_active) {
      auto threads = m_jvm->getThreads();
      threads->forEach(lambda);
      this->processTraces();
      this_thread::sleep_for(1000ms);
    }
    m_jvm->dettachThread();
    logDebug("end profile thread");
  });
}

void kotlinTracer::Profiler::stop() {
  logDebug("stop profiler");
  m_active = false;
  m_profilerThread->join();
}

void kotlinTracer::Profiler::traceStart(jlong t_coroutineId) {
  auto charBuffer = make_unique<char[]>(100);
  pthread_getname_np(pthread_self(), charBuffer.get(), 100);
  auto start = kotlinTracer::currentTimeNs();
  kotlinTracer::TraceInfo trace_info{t_coroutineId, start, 0};
  m_storage.createChildCoroutineStorage(t_coroutineId);
  if (m_storage.addOngoingTraceInfo(trace_info)) {
    logDebug("trace start: " + to_string(start) + " coroutine:" + to_string(t_coroutineId));
  } else {
    throw runtime_error("Found trace that already started");
  }
}

void kotlinTracer::Profiler::traceEnd(jlong t_coroutineId) {
  auto finish = kotlinTracer::currentTimeNs();
  auto traceInfo = findOngoingTrace(t_coroutineId);
  traceInfo.end = finish;
  removeOngoingTrace(traceInfo.coroutineId);
  auto elapsedTime = traceInfo.end - traceInfo.start;
  logDebug("trace end: " + to_string(finish) + ":" + to_string(t_coroutineId) + " time: "
               + to_string(elapsedTime));
  logDebug("threshold: " + to_string(m_threshold.count()));
  if (elapsedTime > m_threshold.count()) {
    std::stringstream output;
    output << "[Kotlin-tracer]===============================================\n";
    output << "[Kotlin-tracer] Trace info: \n";
    output << "[Kotlin-tracer] Coroutine id: " << t_coroutineId << "\n";
    output << "[Kotlin-tracer] Time: " << elapsedTime << "\n";
    printSuspensions(t_coroutineId, output);
    output << "[Kotlin-tracer]===============================================";
    cout << output.str() << endl;
  }
}

void kotlinTracer::Profiler::printSuspensions(jlong t_coroutineId, std::stringstream &output) {
  auto suspensions = m_storage.getSuspensions(t_coroutineId);
  std::function<void(shared_ptr<SuspensionInfo>)> suspensionPrint = [&output](const shared_ptr<SuspensionInfo> &suspension) {
    output << "[Kotlin-tracer]===============================================\n";
    output << "[Kotlin-tracer] Suspended time: " << to_string(suspension->end - suspension->start) << "ns\n";
    output << "[Kotlin-tracer] Suspend stack trace: \n";
    for (auto &frame : *suspension->suspensionStackTrace) {
      output << *frame << "\n";
    }
  };
  if (suspensions != nullptr) {
    output << "[Kotlin-tracer] Suspensions count: " << suspensions->size() << "\n";
    suspensions->forEach(suspensionPrint);
  } else {
    output << "[Kotlin-tracer] Suspensions count: 0\n";
  }
  if (m_storage.containsChildCoroutineStorage(t_coroutineId)) {
    std::function<void(jlong)> childCoroutinePrint = [&output, this](jlong childCoroutine) {
      printSuspensions(childCoroutine, output);
    };
    if (m_storage.containsChildCoroutineStorage(t_coroutineId)) {
      auto children = m_storage.getChildCoroutines(t_coroutineId);
      output << "[Kotlin-tracer] Children count: " << children->size()  << "\n";
      children->forEach(childCoroutinePrint);
    } else {
      output << "[Kotlin-tracer] Children count: 0\n";
    }
  }
}

void kotlinTracer::Profiler::removeOngoingTrace(const jlong &coroutineId) {
  m_storage.removeOngoingTraceInfo(coroutineId);
}

kotlinTracer::TraceInfo &kotlinTracer::Profiler::findOngoingTrace(const jlong &coroutineId) {
  return m_storage.findOngoingTraceInfo(coroutineId);
}

void kotlinTracer::Profiler::processTraces() {
  auto rawRecord = m_storage.removeRawTraceHeader();
  while (rawRecord != nullptr) {
    auto processedRecord = make_shared<ProcessedTraceRecord>(ProcessedTraceRecord{});
    auto thread = m_jvm->findThread(rawRecord->thread);
    if (thread == nullptr) logInfo("Cannot get thread: " + *thread->name);
    processedRecord->time = rawRecord->time;
    processedRecord->thread_name = thread->name;
    processedRecord->trace_count = rawRecord->trace_count;
    shared_ptr<list<shared_ptr<string>>> methods = make_shared<list<shared_ptr<string>>>();
    for (int i = 0; i < rawRecord->trace_count; i++) {
      ASGCTCallFrame frame = rawRecord->trace->frames[i];
      methods->push_back(processMethodInfo(frame.methodId, frame.lineno));
    }
    processedRecord->methodInfo = std::move(methods);
    m_storage.addProcessedTrace(processedRecord);
    rawRecord = m_storage.removeRawTraceHeader();
  }

}

shared_ptr<string> kotlinTracer::Profiler::processMethodInfo(jmethodID methodId, jint lineno) {
  if (!m_methodInfoMap.contains(methodId)) {
    auto jvmti = m_jvm->getJvmTi();
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
    m_methodInfoMap.insert(methodId, method);
    return make_shared<string>(*method + ':' + line);
  } else {
    string line = to_string(lineno);
    return make_shared<string>(*m_methodInfoMap.get(methodId) + ':' + line);
  }
}

std::string kotlinTracer::Profiler::tickToMessage(jint t_ticks) {
  switch (t_ticks) {
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

void kotlinTracer::Profiler::coroutineCreated(jlong t_coroutineId, jlong t_parentId) {
  if (m_storage.containsChildCoroutineStorage(t_parentId)) {
    m_storage.addChildCoroutine(t_coroutineId, t_parentId);
  }
}

void kotlinTracer::Profiler::coroutineSuspended(jlong t_coroutineId) {
  kotlinTracer::coroutineSuspended(t_coroutineId);
  ::jthread thread;
  jvmtiFrameInfo frames[20];
  jint framesCount;
  auto jvmti = m_jvm->getJvmTi();
  jvmti->GetCurrentThread(&thread);
  jvmtiError err = jvmti->GetStackTrace(thread, 0, size(frames), frames, &framesCount);
  if (err == JVMTI_ERROR_NONE && framesCount >= 1) {
    auto suspensionInfo = make_shared<SuspensionInfo>(SuspensionInfo{t_coroutineId, currentTimeNs(), 0,
                                                                     make_unique<list<shared_ptr<string>>>()});
    for (int i = 0; i < framesCount; ++i) {
      suspensionInfo->suspensionStackTrace->push_back(
          processMethodInfo(frames[i].method, (jint) frames[i].location));
    }
    m_storage.addSuspensionInfo(suspensionInfo);
  }
}

void kotlinTracer::Profiler::coroutineResumed(jlong t_coroutineId) {
  kotlinTracer::coroutineResumed(t_coroutineId);
  m_coroutineId = t_coroutineId;
  auto suspensionInfo = m_storage.getLastSuspensionInfo(t_coroutineId);
  if (suspensionInfo != nullptr) {
    suspensionInfo->end = currentTimeNs();
  }
}

void kotlinTracer::Profiler::coroutineCompleted(jlong t_coroutineId) {
  kotlinTracer::coroutineCompleted();
}
