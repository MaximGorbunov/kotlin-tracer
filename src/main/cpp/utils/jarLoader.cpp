#include <filesystem>
#include <iostream>
#include <memory>

#include "jarLoader.hpp"

using namespace std;

unique_ptr<kotlinTracer::InstrumentationMetadata> kotlinTracer::JarLoader::load(const std::string &t_name,
                                                                                std::shared_ptr<JVM> t_jvm) {
  auto jarPath = absolute(filesystem::current_path().append(t_name)).string();
  auto err = t_jvm->getJvmTi()->AddToSystemClassLoaderSearch(jarPath.c_str());
  if (err != JVMTI_ERROR_NONE) {
    throw runtime_error("Cannot add jar to system classloader search:" + to_string(err));

  }
  auto jni = t_jvm->getJNIEnv();
  jclass inst = jni->FindClass("io/inst/CoroutineInstrumentator");
  if (inst == nullptr) {
    cout << "Cannot load instrumentation class" << endl;
    throw runtime_error("Cannot get instrumentator class!");
  }

  jmethodID transformKotlinCoroutines = jni->GetStaticMethodID(inst, "transformKotlinCoroutines",
                                                               "([B)[B");  // will resolve class
  jmethodID transformMethod = jni->GetStaticMethodID(inst, "transformMethodForTracing",
                                                     "([BLjava/lang/String;)[B");  // will resolve class
  if (transformKotlinCoroutines == nullptr) {
    cout << "Could not resolve transform coroutine method" << endl;
    jni->ExceptionDescribe();
  }
  if (transformMethod == nullptr) {
    cout << "Could not resolve transform tracing method" << endl;
    jni->ExceptionDescribe();
  }
  return std::make_unique<InstrumentationMetadata>(InstrumentationMetadata{inst, transformKotlinCoroutines,
                                                                           transformMethod});
}