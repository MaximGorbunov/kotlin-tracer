#include <filesystem>
#include <iostream>
#include <memory>

#include "jarLoader.hpp"
#include "log.h"

using std::string, std::to_string, std::runtime_error, std::unique_ptr,
    std::filesystem::current_path, std::filesystem::path;

namespace kotlin_tracer {
unique_ptr<kotlin_tracer::InstrumentationMetadata> load(
    std::unique_ptr<std::string> jar_path,
    const std::string &name,
    JVM* jvm
) {
  auto jarPath = jar_path == nullptr
                 ? absolute(current_path() / name).string()
                 : absolute(path(*jar_path)).string();
  logDebug("Adding jar: " + jarPath + " to the system classpath");
  auto err = jvm->getJvmTi()->AddToSystemClassLoaderSearch(jarPath.c_str());
  if (err != JVMTI_ERROR_NONE) {
    throw runtime_error(
        "Cannot add jar to system classloader search:" + to_string(err));
  }
  auto jni = jvm->getJNIEnv();
  jclass inst = jni->FindClass("io/inst/CoroutineInstrumentator");
  // Create global reference in order to prevent unloading of class
  auto klassReference = (jclass) jni->NewGlobalRef(inst);
  if (inst == nullptr) {
    logError("Cannot load instrumentation class");
    throw runtime_error("Cannot get instrumentator class!");
  }

  jmethodID transformKotlinCoroutines =
      jni->GetStaticMethodID(inst, "transformKotlinCoroutines",
                             "([B)[B");  // will resolve class
  jmethodID transformMethod =
      jni->GetStaticMethodID(inst, "transformMethodForTracing",
                             "([BLjava/lang/String;)[B");  // will resolve class
  if (transformKotlinCoroutines == nullptr) {
    logError("Could not resolve transform coroutine method");
    jni->ExceptionDescribe();
  }
  if (transformMethod == nullptr) {
    logError("Could not resolve transform tracing method");
    jni->ExceptionDescribe();
  }
  return std::make_unique<InstrumentationMetadata>(InstrumentationMetadata{
      klassReference,
      transformKotlinCoroutines,
      transformMethod});
}
}  // namespace kotlin_tracer
