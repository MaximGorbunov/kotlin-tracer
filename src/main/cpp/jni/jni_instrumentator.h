/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class io_inst_CoroutineInstrumentator */

#ifndef _Included_io_inst_CoroutineInstrumentator
#define _Included_io_inst_CoroutineInstrumentator
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    coroutineCreated
 * Signature: (JJ)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_coroutineCreated(JNIEnv *, jclass, jlong);

/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    coroutineResumed
 * Signature: (J)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_coroutineResumed(JNIEnv *, jclass, jlong);

/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    coroutineCompleted
 * Signature: (J)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_coroutineCompleted(JNIEnv *, jclass, jlong);

/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    coroutineSuspend
 * Signature: (J)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_coroutineSuspend(JNIEnv *, jclass, jlong);

/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    traceStart
 * Signature: (J)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_traceStart(JNIEnv *, jclass);

/*
 * Class:     io_inst_CoroutineInstrumentator
 * Method:    traceEnd
 * Signature: (J)V
 */
[[maybe_unused]]
JNIEXPORT void JNICALL Java_io_inst_CoroutineInstrumentator_traceEnd(JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif
