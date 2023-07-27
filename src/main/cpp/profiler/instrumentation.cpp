#include <cstring>
#include "instrumentation.hpp"
#include "../utils/log.h"

namespace kotlin_tracer {

using std::string;

void Instrumentation::instrument(JNIEnv *jni_env,
                                 const char *name,
                                 jint class_data_len,
                                 const unsigned char *class_data,
                                 jint *new_class_data_len,
                                 unsigned char **new_class_data) {
  if (!strcmp(profiler_options_->class_name->c_str(), name)) {
    logDebug("Found tracing class!" + *profiler_options_->class_name);
    jbyteArray transformed = instrumentMethod(
        class_data,
        class_data_len,
        *profiler_options_->method_name);
    jboolean copy = false;
    auto *new_code =
        (unsigned char *) jni_env->GetByteArrayElements(transformed, &copy);
    *new_class_data_len = jni_env->GetArrayLength(transformed);
    *new_class_data = new_code;
  }
  if (!strcmp(KOTLIN_COROUTINE_PROBES_NAME, name)) {
    jbyteArray transformed =
        instrumentCoroutines(class_data, class_data_len);
    jboolean copy = false;
    if (transformed == nullptr) return;
    auto *new_code =
        (unsigned char *) jni_env->GetByteArrayElements(transformed, &copy);
    *new_class_data_len = jni_env->GetArrayLength(transformed);
    *new_class_data = new_code;
  }
}

void Instrumentation::setInstrumentationMetadata(
    std::unique_ptr<InstrumentationMetadata> metadata
) {
  instrumentation_metadata_ = std::move(metadata);
}

jbyteArray Instrumentation::instrumentCoroutines(
    const unsigned char *byte_code,
    int size
) {
  JNIEnv *jni = jvm_->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  jni->SetByteArrayRegion(
      array,
      0,
      size,
      reinterpret_cast<const jbyte *>(byte_code));
  return reinterpret_cast<jbyteArray>(jni->CallStaticObjectMethod(
      instrumentation_metadata_->klass,
      instrumentation_metadata_->instrument_coroutines_method,
      array));
}

jbyteArray Instrumentation::instrumentMethod(
    const unsigned char *byte_code,
    int size,
    const string &method_name
) {
  JNIEnv *jni = jvm_->getJNIEnv();
  jbyteArray array = jni->NewByteArray(size);
  auto jMethodName = jni->NewStringUTF(method_name.c_str());
  jni->SetByteArrayRegion(
      array,
      0,
      size,
      reinterpret_cast<const jbyte *>(byte_code));
  return reinterpret_cast<jbyteArray>(jni->CallStaticObjectMethod(
      instrumentation_metadata_->klass,
      instrumentation_metadata_->instrument_method,
      array,
      jMethodName));
}

std::unique_ptr<string> Instrumentation::getJarPath() {
  if (profiler_options_->jar_path == nullptr) {
    return {nullptr};
  } else {
    return make_unique<string>(*profiler_options_->jar_path);
  }
}

}  // namespace kotlin_tracer
