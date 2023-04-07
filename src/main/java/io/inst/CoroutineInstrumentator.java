
package io.inst;

import io.inst.javassist.CannotCompileException;
import io.inst.javassist.ClassPool;
import io.inst.javassist.CtClass;
import io.inst.javassist.CtMethod;
import io.inst.javassist.NotFoundException;
import io.inst.javassist.bytecode.BadBytecode;
import io.inst.javassist.bytecode.CodeIterator;
import io.inst.javassist.bytecode.ConstPool;
import io.inst.javassist.bytecode.Opcode;

import java.io.ByteArrayInputStream;

public class CoroutineInstrumentator {
    private static final ClassPool pool = ClassPool.getDefault();

    public static byte[] transformKotlinCoroutines(byte[] clazz) {

        try (ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(clazz)) {
            CtClass ctClass = pool.makeClass(byteArrayInputStream);
            CtMethod probeCoroutineCreated = ctClass.getDeclaredMethod("probeCoroutineCreated$kotlinx_coroutines_core");
            CtMethod probeCoroutineResumed = ctClass.getDeclaredMethod("probeCoroutineResumed$kotlinx_coroutines_core");
            CtMethod probeCoroutineSuspended = ctClass.getDeclaredMethod(
                "probeCoroutineSuspended$kotlinx_coroutines_core");
            CtMethod probeCoroutineCompleted = ctClass.getDeclaredMethod("access$probeCoroutineCompleted");
            String coroutineCreatedSrc = "System.out.println(\"CoroutineCreated\");kotlin.coroutines.CoroutineContext context = completion.getContext();\n" +
                                         "kotlinx.coroutines.CoroutineId coroutineName = (kotlinx.coroutines" +
                                         ".CoroutineId)context.get((kotlin.coroutines.CoroutineContext.Key)kotlinx" +
                                         ".coroutines.CoroutineId.Key);\n" +
                                         "io.inst.CoroutineInstrumentator.coroutineCreated(coroutineName.getId(), io" +
                                         ".inst.span.CoroutineTrace.getCurrentCoroutine());";
            String coroutineSuspendedSrc = "kotlin.coroutines.CoroutineContext context = frame.getContext();\n" +
                                           "kotlinx.coroutines.Job job = (kotlinx.coroutines.Job) context.get((kotlin" +
                                           ".coroutines.CoroutineContext.Key)kotlinx.coroutines.Job.Key);\n" +
                                           "kotlinx.coroutines.CoroutineId coroutineName = (kotlinx.coroutines" +
                                           ".CoroutineId)context.get((kotlin.coroutines.CoroutineContext.Key)kotlinx" +
                                           ".coroutines.CoroutineId.Key);\n" +
                                           "io.inst.CoroutineInstrumentator.coroutineSuspend(coroutineName.getId());";
            String coroutineResumedSrc = "kotlin.coroutines.CoroutineContext context = frame.getContext();\n" +
                                         "kotlinx.coroutines.CoroutineId coroutineName = (kotlinx.coroutines" +
                                         ".CoroutineId)context.get((kotlin.coroutines.CoroutineContext.Key)kotlinx" +
                                         ".coroutines.CoroutineId.Key);\n" +
                                         "io.inst.span.CoroutineTrace.coroutineResume(coroutineName.getId());\n;" +
                                         "io.inst.CoroutineInstrumentator.coroutineResumed(coroutineName.getId());";
            String coroutineCompletedSrc = "kotlin.coroutines.CoroutineContext context = owner.info.getContext();\n" +
                                           "kotlinx.coroutines.CoroutineId coroutineName = (kotlinx.coroutines" +
                                           ".CoroutineId)context.get((kotlin.coroutines.CoroutineContext.Key)kotlinx" +
                                           ".coroutines.CoroutineId.Key);\n" +
                                           "io.inst.CoroutineInstrumentator.coroutineCompleted(coroutineName.getId());";
            probeCoroutineCreated.insertBefore(coroutineCreatedSrc);
            probeCoroutineSuspended.insertBefore(coroutineSuspendedSrc);
            probeCoroutineCompleted.insertBefore(coroutineCompletedSrc);
            probeCoroutineResumed.insertBefore(coroutineResumedSrc);
            return ctClass.toBytecode();
        } catch (Throwable e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    public static byte[] transformMethodForTracing(byte[] clazz, String methodName) {
        String codeAtMethodStart =
            "spanId = io.inst.CoroutineInstrumentator.traceStart(io.inst.span.CoroutineTrace.getCurrentCoroutine());";
        String codeAtMethodEnd =
            "io.inst.CoroutineInstrumentator.traceEnd(io.inst.span.CoroutineTrace.getCurrentCoroutine(), spanId);";
        try (ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(clazz)) {
            CtClass ctClass = pool.makeClass(byteArrayInputStream);
            CtMethod method = ctClass.getDeclaredMethod(methodName);
            boolean suspendFunction = isSuspendFunction(method);
            if (suspendFunction) {
                initializeSpanIdVariable(method);
                instrumentSuspendFunction(method, codeAtMethodStart);
            } else {
                method.insertBefore(codeAtMethodStart);
                initializeSpanIdVariable(method);
            }
            method.insertAfterLastReturn(codeAtMethodEnd, false, true);
            return ctClass.toBytecode();
        } catch (Throwable e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    private static void instrumentSuspendFunction(CtMethod method, String src)
        throws BadBytecode, CannotCompileException {
        CodeIterator iterator = method.getMethodInfo().getCodeAttribute().iterator();
        int handlerPos = iterator.getCodeLength();
        int previousPos = -1;
        while (iterator.hasNext()) {
            int pos = iterator.next();
            if (pos >= handlerPos) {
                break;
            }

            int byteCode = iterator.byteAt(pos);
            if (byteCode == Opcode.TABLESWITCH && isCoroutineLabelSwitch(method, iterator, previousPos)) {
                int branchPosition = getFirstBranchPosition(iterator, pos);
                System.out.println("First branch position:" + branchPosition);
                method.insertAtPoisition(branchPosition + 1, src);
            }
            previousPos = pos;
        }
    }

    private static int getFirstBranchPosition(CodeIterator iterator, int pos) {
        int index = (pos & ~3) + 4;
        index += 12;
        return iterator.s32bitAt(index) + pos;
    }

    private static boolean isCoroutineLabelSwitch(CtMethod method, CodeIterator iterator, int previousPos) {
        if (previousPos != -1 && iterator.byteAt(previousPos) == Opcode.GETFIELD) {
            ConstPool pool = method.getMethodInfo2().getConstPool();
            int index = iterator.u16bitAt(previousPos + 1);
            String className = pool.getFieldrefClassName(index);
            String fieldName = pool.getFieldrefName(index);
            String fieldType = pool.getFieldrefType(index);

            String classQualifier = method.getDeclaringClass().getName() + "$" + method.getName();
            return className != null && className.startsWith(classQualifier) && "label".equals(fieldName) && "I".equals(
                fieldType);
        }
        return false;
    }

    private static boolean isSuspendFunction(CtMethod method) throws NotFoundException {
        boolean suspendFunction = false;
        CtClass[] types = method.getParameterTypes();
        for (int i = 0; i < types.length; i++) {
            if ("kotlin.coroutines.Continuation".equals(types[i].getName())) {
                suspendFunction = true;
            }
        }
        return suspendFunction;
    }

    private static void initializeSpanIdVariable(CtMethod method) throws CannotCompileException, NotFoundException {
        method.addLocalVariable("spanId", pool.get("long"));
        method.insertBefore("spanId = -1;");
    }

    public static native void coroutineCreated(long coroutineId, long parentCoroutineId);

    public static native void coroutineResumed(long coroutineId);

    public static native void coroutineCompleted(long coroutineId);

    public static native void coroutineSuspend(long coroutineId);

    public static native void traceStart(long coroutineId);

    public static native void traceEnd(long coroutineId, long spanId);
}
