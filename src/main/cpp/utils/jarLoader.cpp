#include "jarLoader.h"
#include <filesystem>
#include <iostream>
#include "jvm.h"

using namespace std;

void JarLoader::load(const std::string &name) {
  auto jarPath = absolute(filesystem::current_path().append(name)).string();
  auto jvm = JVM::getInstance();
  auto err = jvm->getJvmTi()->AddToSystemClassLoaderSearch(jarPath.c_str());
  if (err != JVMTI_ERROR_NONE) {
    throw runtime_error("Cannot add jar to system classloader search:" + to_string(err));

  }
  auto jni = jvm->getJNIEnv();
  jclass inst = jni->FindClass("io/inst/CoroutineInstrumentator");
  if (inst == nullptr) {
    cout << "Cannot load instrumentation class" << endl;
    throw runtime_error("Cannot find instrumentator class!");
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
  InstrumentationMetadata metadata = {inst, transformKotlinCoroutines, transformMethod};
  jvm->setInstrumentationMetadata(metadata);
}