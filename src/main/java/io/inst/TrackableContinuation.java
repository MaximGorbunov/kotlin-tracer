package io.inst;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import kotlin.coroutines.Continuation;
import kotlin.coroutines.CoroutineContext;
import org.jetbrains.annotations.NotNull;

public class TrackableContinuation<T> implements Continuation<T> {
    private static final Method idGetter;
    private static final CoroutineContext.Key<?> key;

    static {
        try {
            Class<?> coroutineIdClass = Class.forName("kotlinx.coroutines.CoroutineId");
            idGetter = coroutineIdClass.getDeclaredMethod("getId");
            key = (CoroutineContext.Key<?>)coroutineIdClass.getDeclaredField("Key").get(null);
        } catch (ClassNotFoundException | NoSuchMethodException | NoSuchFieldException | IllegalAccessException e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    private Continuation continuation;

    public TrackableContinuation(Continuation parent) {
        this.continuation = parent;
    }

    @NotNull
    @Override
    public CoroutineContext getContext() {
        return continuation.getContext();
    }

    @Override
    public void resumeWith(@NotNull Object o) {
        CoroutineContext context = getContext();
        Object coroutineId = context.get(key);
        continuation.resumeWith(o);
        try {
            CoroutineInstrumentator.traceEnd((Long)idGetter.invoke(coroutineId));
        } catch (IllegalAccessException | InvocationTargetException e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }

    }
}