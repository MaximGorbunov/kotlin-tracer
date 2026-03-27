#include "instrumentation.hpp"

#include <cstring>

#include "../utils/log.h"

namespace kotlin_tracer {

using std::string;

void Instrumentation::instrument(jvmtiEnv* jvmti_env, JNIEnv* jni_env, const char* name,
                                 jint class_data_len, const unsigned char* class_data,
                                 jint* new_class_data_len, unsigned char** new_class_data) {
  if (!strcmp(profiler_options_->class_name->c_str(), name)) {
    logDebug("Found tracing class!" + *profiler_options_->class_name);
    jbyteArray transformed = instrumentMethod(class_data, class_data_len,
                                              *profiler_options_->method_name);
    auto length = jni_env->GetArrayLength(transformed);
    unsigned char* buf = nullptr;
    if (jvmti_env->Allocate(length, &buf) != JVMTI_ERROR_NONE) {
      throw std::runtime_error("Failed to allocate JVMTI buffer for tracing class");
    }
    jni_env->GetByteArrayRegion(transformed, 0, length, reinterpret_cast<jbyte*>(buf));
    if (jni_env->ExceptionCheck()) {
      jni_env->ExceptionDescribe();
      jni_env->ExceptionClear();
      throw std::runtime_error("Failed to copy transformed traced class content");
    }
    *new_class_data_len = length;
    *new_class_data = buf;
  }
  if (!strcmp(KOTLIN_COROUTINE_PROBES_NAME, name)) {
    jbyteArray transformed = instrumentCoroutines(class_data, class_data_len);
    auto length = jni_env->GetArrayLength(transformed);
    unsigned char* buf = nullptr;
    if (jvmti_env->Allocate(length, &buf) != JVMTI_ERROR_NONE) {
      throw std::runtime_error("Failed to allocate JVMTI buffer for coroutine probes class");
    }
    jni_env->GetByteArrayRegion(transformed, 0, length, reinterpret_cast<jbyte*>(buf));
    if (jni_env->ExceptionCheck()) {
      jni_env->ExceptionDescribe();
      jni_env->ExceptionClear();
      throw std::runtime_error("Failed to copy transformed coroutine class content");
    }
    *new_class_data_len = jni_env->GetArrayLength(transformed);
    *new_class_data = buf;
  }
}

void Instrumentation::setInstrumentationMetadata(
    std::unique_ptr<InstrumentationMetadata> metadata) {
  instrumentation_metadata_ = std::move(metadata);
}

jbyteArray Instrumentation::instrumentCoroutines(const unsigned char* byte_code, int size) {
  JNIEnv* jni = jvm_->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to create new byte array transformed coroutine class content");
  }
  jni->SetByteArrayRegion(array, 0, size, reinterpret_cast<const jbyte*>(byte_code));
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to set byte array region");
  }
  auto result = jni->CallStaticObjectMethod(instrumentation_metadata_->klass,
                                            instrumentation_metadata_->instrument_coroutines_method,
                                            array);
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to intrusment coroutines");
  }
  auto instrumented_class = reinterpret_cast<jbyteArray>(result);
  return instrumented_class;
}

jbyteArray Instrumentation::instrumentMethod(const unsigned char* byte_code, int size,
                                             const string& method_name) {
  JNIEnv* jni = jvm_->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to create new byte array transformed traced class content");
  }
  auto jMethodName = jni->NewStringUTF(method_name.c_str());
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to create new string with traced method name");
  }
  jni->SetByteArrayRegion(array, 0, size, reinterpret_cast<const jbyte*>(byte_code));
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to set byte array region");
  }
  auto instrumentation_result = reinterpret_cast<jbyteArray>(jni->CallStaticObjectMethod(
      instrumentation_metadata_->klass, instrumentation_metadata_->instrument_method, array,
      jMethodName));
  if (jni->ExceptionCheck()) {
    jni->ExceptionDescribe();
    jni->ExceptionClear();
    throw std::runtime_error("Failed to instrument method");
  }
  return instrumentation_result;
}

std::unique_ptr<string> Instrumentation::getJarPath() {
  if (profiler_options_->jar_path == nullptr) {
    return {nullptr};
  } else {
    return make_unique<string>(*profiler_options_->jar_path);
  }
}

}  // namespace kotlin_tracer
