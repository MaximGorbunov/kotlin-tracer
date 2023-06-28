
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
import io.inst.javassist.expr.ExprEditor;
import io.inst.javassist.expr.MethodCall;
import java.io.ByteArrayInputStream;

import static io.inst.javassist.CtBehavior.isCoroutineLabelSwitch;

public class CoroutineInstrumentator {
    private static final ClassPool pool = ClassPool.getDefault();

    public static byte[] transformKotlinCoroutines(byte[] clazz) {
        System.out.println("Inside method");
        try (ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(clazz)) {
            CtClass ctClass = pool.makeClass(byteArrayInputStream);
            CtMethod probeCoroutineCreated = ctClass.getDeclaredMethod("probeCoroutineCreated$kotlinx_coroutines_core");
            CtMethod probeCoroutineResumed = ctClass.getDeclaredMethod("probeCoroutineResumed$kotlinx_coroutines_core");
            CtMethod probeCoroutineSuspended = ctClass.getDeclaredMethod("probeCoroutineSuspended$kotlinx_coroutines_core");
            CtMethod probeCoroutineCompleted = ctClass.getDeclaredMethod("access$probeCoroutineCompleted");
            String coroutineCreatedSrc = "kotlin.coroutines.CoroutineContext context = completion.getContext();\n" +
                                         "kotlinx.coroutines.CoroutineId coroutineName = (kotlinx.coroutines" +
                                         ".CoroutineId)context.get((kotlin.coroutines.CoroutineContext.Key)kotlinx" +
                                         ".coroutines.CoroutineId.Key);\n" +
                                         "io.inst.CoroutineInstrumentator.coroutineCreated(coroutineName.getId());";
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
                "io.inst.CoroutineInstrumentator.traceStart();";
        String codeAtMethodEnd =
                "io.inst.CoroutineInstrumentator.traceEnd(-2L);";
        try (ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(clazz)) {
            CtClass ctClass = pool.makeClass(byteArrayInputStream);
            CtMethod method = ctClass.getDeclaredMethod(methodName);
            boolean suspendFunction = isSuspendFunction(method);
            if (suspendFunction) {
                instrumentSuspendFunction(method, codeAtMethodStart, codeAtMethodEnd);
            } else {
                method.insertBefore(codeAtMethodStart);
                method.insertAfter(codeAtMethodEnd, false, true);
            }
            return ctClass.toBytecode();
        } catch (Throwable e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    private static void replaceContinuation(CtMethod method) throws NotFoundException, CannotCompileException {
        CtClass continuationClass = method.getDeclaringClass().getClassPool().get("kotlin.coroutines.Continuation");
        method.instrument(new ExprEditor() {
            @Override
            public void edit(MethodCall m) throws CannotCompileException {
                try {
                    CtMethod invokedMethod = m.getMethod();
                    CtClass[] parameterTypes = invokedMethod.getParameterTypes();
                    int continuationParameterIndex = -1;
                    for (int i = 0; i < parameterTypes.length; i++) {
                        if (parameterTypes[i].subtypeOf(continuationClass)) {
                            continuationParameterIndex = i + 1;
                            break;
                        }
                    }
                    if (continuationParameterIndex > 0) {
                        String statement = "{" +
                                           "$" + continuationParameterIndex + "= new io.inst.TrackableContinuation($" + continuationParameterIndex + ");" +
                                           "$_ = $proceed($$);" +
                                           "}";
                        m.replace(statement);
                    }
                } catch (NotFoundException e) {
                    throw new RuntimeException(e);
                }
            }
        });
    }

    private static void instrumentSuspendFunction(CtMethod method, String src, String codeAtMethodEnd)
            throws BadBytecode, CannotCompileException, NotFoundException {
        CodeIterator iterator = method.getMethodInfo().getCodeAttribute().iterator();
        int handlerPos = iterator.getCodeLength();
        int previousPos = -1;
        boolean containsTableSwitch = false;
        while (iterator.hasNext()) {
            int pos = iterator.next();
            if (pos >= handlerPos) {
                break;
            }

            int byteCode = iterator.byteAt(pos);
            if (byteCode == Opcode.TABLESWITCH && isCoroutineLabelSwitch(method.getDeclaringClass(), method.getMethodInfo2(), iterator, previousPos)) {
                containsTableSwitch = true;
                int branchPosition = getFirstBranchPosition(iterator, pos);
                method.insertAtPoisition(branchPosition, src);
            }
            previousPos = pos;
        }
        if (!containsTableSwitch) {
            method.insertBefore(src);
            replaceContinuation(method);
        } else {
            method.insertAfterLastSwitchTableBranch(codeAtMethodEnd, false, true);
        }
    }

    private static int getFirstBranchPosition(CodeIterator iterator, int pos) {
        int index = (pos & ~3) + 4;
        index += 12;
        return iterator.s32bitAt(index) + pos;
    }

    private static boolean isSuspendFunction(CtMethod method) throws NotFoundException {
        boolean suspendFunction = false;
        CtClass[] types = method.getParameterTypes();
        for (CtClass type : types) {
            if ("kotlin.coroutines.Continuation".equals(type.getName())) {
                suspendFunction = true;
                break;
            }
        }
        return suspendFunction;
    }

    public static native void coroutineCreated(long coroutineId);

    public static native void coroutineResumed(long coroutineId);

    public static native void coroutineCompleted(long coroutineId);

    public static native void coroutineSuspend(long coroutineId);

    public static native void traceStart();

    public static native void traceEnd(long coroutineId);
}
