package io.inst.span;

public class CoroutineTrace {
    private static final ThreadLocal<Long> lastCoroutineId = ThreadLocal.withInitial(() -> -1L);

    public static void coroutineResume(long coroutineId) {
        lastCoroutineId.set(coroutineId);
    }

    public static long getCurrentCoroutine() {
        return lastCoroutineId.get();
    }
}

