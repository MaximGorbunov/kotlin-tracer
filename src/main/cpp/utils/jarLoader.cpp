#include <filesystem>
#include <iostream>
#include <memory>

#include "jarLoader.hpp"
#include "log.h"

using std::string, std::to_string, std::runtime_error, std::unique_ptr, std::filesystem::current_path, std::filesystem::path;

namespace kotlin_tracer {
unique_ptr<kotlin_tracer::InstrumentationMetadata> kotlin_tracer::JarLoader::load(unique_ptr<string> t_jar_path,
                                                                                  const std::string &t_name,
                                                                                  const std::shared_ptr<JVM> &t_jvm) {
  auto jarPath = t_jar_path == nullptr
                 ? absolute(current_path() / t_name).string()
                 : absolute(path(*t_jar_path)).string();
  logDebug("Adding jar: " + jarPath + " to the system classpath");
  auto err = t_jvm->getJvmTi()->AddToSystemClassLoaderSearch(jarPath.c_str());
  if (err != JVMTI_ERROR_NONE) {
    throw runtime_error("Cannot add jar to system classloader search:" + to_string(err));
  }
  auto jni = t_jvm->getJNIEnv();
  jclass inst = jni->FindClass("io/inst/CoroutineInstrumentator");
  auto klassReference =
      (jclass) jni->NewGlobalRef(inst); // Create global reference in order to prevent unloading of class
  if (inst == nullptr) {
    logError("Cannot load instrumentation class");
    throw runtime_error("Cannot get instrumentator class!");
  }

  jmethodID transformKotlinCoroutines = jni->GetStaticMethodID(inst, "transformKotlinCoroutines",
                                                               "([B)[B");  // will resolve class
  jmethodID transformMethod = jni->GetStaticMethodID(inst, "transformMethodForTracing",
                                                     "([BLjava/lang/String;)[B");  // will resolve class
  if (transformKotlinCoroutines == nullptr) {
    logError("Could not resolve transform coroutine method");
    jni->ExceptionDescribe();
  }
  if (transformMethod == nullptr) {
    logError("Could not resolve transform tracing method");
    jni->ExceptionDescribe();
  }
  return std::make_unique<InstrumentationMetadata>(InstrumentationMetadata{klassReference, transformKotlinCoroutines,
                                                                           transformMethod});
}
}