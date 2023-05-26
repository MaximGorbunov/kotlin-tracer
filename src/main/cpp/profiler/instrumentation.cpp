#include <cstring>
#include "instrumentation.hpp"
#include "../utils/log.h"

using namespace kotlinTracer;

const std::string Instrumentation::KOTLIN_COROUTINE_PROBES_NAME = "kotlinx/coroutines/debug/internal/DebugProbesImpl";

void Instrumentation::instrument(JNIEnv *jniEnv,
                                 const char *t_name,
                                 jint t_classDataLen,
                                 const unsigned char *t_classData,
                                 jint *t_newClassDataLen,
                                 unsigned char **t_newClassData) {
  if (!strcmp(m_profilerOptions->className->c_str(), t_name)) {
    logDebug("Found tracing class!" + *m_profilerOptions->className);
    jbyteArray transformed = instrumentMethod(t_classData, t_classDataLen, *m_profilerOptions->methodName);
    jboolean copy = false;
    auto *new_code =
        (unsigned char *) jniEnv->GetByteArrayElements(transformed, &copy);
    *t_newClassDataLen = jniEnv->GetArrayLength(transformed);
    *t_newClassData = new_code;
  }
  if (!strcmp(KOTLIN_COROUTINE_PROBES_NAME.c_str(), t_name)) {
    jbyteArray transformed =
        instrumentCoroutines(t_classData, t_classDataLen);
    jboolean copy = false;
    auto *new_code =
        (unsigned char *) jniEnv->GetByteArrayElements(transformed, &copy);
    *t_newClassDataLen = jniEnv->GetArrayLength(transformed);
    *t_newClassData = new_code;
  }
}

void Instrumentation::setInstrumentationMetadata(std::unique_ptr<InstrumentationMetadata> metadata) {
  m_instrumentMetadata = std::move(metadata);
}

jbyteArray Instrumentation::instrumentCoroutines(const unsigned char *byteCode, int size) {
  JNIEnv *jni = m_jvm->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  jni->SetByteArrayRegion(array, 0, size, (jbyte *) byteCode);
  return (jbyteArray) jni->CallStaticObjectMethod(m_instrumentMetadata->klass,
                                                  m_instrumentMetadata->instrumentCoroutinesMethod,
                                                  array);
}

jbyteArray Instrumentation::instrumentMethod(const unsigned char *byteCode, int size, const std::string &methodName) {
  JNIEnv *jni = m_jvm->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  auto jMethodName = jni->NewStringUTF(methodName.c_str());
  jni->SetByteArrayRegion(array, 0, size, (jbyte *) byteCode);
  return (jbyteArray) jni->CallStaticObjectMethod(m_instrumentMetadata->klass,
                                                  m_instrumentMetadata->instrumentMethod,
                                                  array,
                                                  jMethodName);
}
std::unique_ptr<std::string> Instrumentation::getJarPath() {
  return std::move(m_profilerOptions->jarPath);
}


