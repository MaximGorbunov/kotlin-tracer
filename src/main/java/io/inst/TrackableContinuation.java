package io.inst;

import kotlin.coroutines.Continuation;
import kotlin.coroutines.CoroutineContext;
import org.jetbrains.annotations.NotNull;

public class TrackableContinuation<T> implements Continuation<T> {
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
        CoroutineInstrumentator.traceEnd();
        continuation.resumeWith(o);
    }
}
