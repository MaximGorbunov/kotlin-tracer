package io.github.maximgorbunov

import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.junit.jupiter.api.Test

class InstrumentationTest {

    @Test
    fun simpleFunctionSwitchTableTest(): Unit = runBlocking {
        switchTableSuspend()
    }

    @Test
    fun complexFunctionSwitchTableTest(): Unit = runBlocking {
        switchTableComplexSuspend(2)
    }

    @Test
    fun simpleFunctionWithoutSwitchTableTest(): Unit = runBlocking {
        suspendWithoutTable()
    }

    @Test
    fun dispatchDefaultTest() = runBlocking {
        launch(start = CoroutineStart.DEFAULT) {
            switchTableComplexSuspend(2)
        }.join()
    }

    @Test
    fun dispatchUndispatchedTest() = runBlocking {
        launch(start = CoroutineStart.UNDISPATCHED) {
            switchTableComplexSuspend(2)
        }.join()
    }

    @Test
    fun dispatchUndispatchedWithoutSuspendTest() = runBlocking {
        launch(start = CoroutineStart.UNDISPATCHED) {
            suspendWithoutSuspensionsFunction()
        }.join()
    }

    @Test
    fun dispatchLazyTest() = runBlocking {
        launch(start = CoroutineStart.LAZY) {
            switchTableComplexSuspend(2)
        }.join()
    }

    @Test
    fun dispatchAtomicTest() = runBlocking {
        launch(start = CoroutineStart.ATOMIC) {
            switchTableComplexSuspend(2)
        }.join()
    }

    @Test
    fun plainFunctionTest() {
        plainFunction()
    }

    fun plainFunction() {
        Thread.sleep(100)
    }

    fun suspendWithoutSuspensionsFunction() {
        Thread.sleep(100)
    }


    suspend fun switchTableSuspend() {
        delay(100)
        println("stub")
    }

    suspend fun switchTableComplexSuspend(a: Int): Int {
        delay(100)
        println("stub")
        delay(100)
        if (a > 0) return 2
        else return 3
    }

    suspend fun suspendWithoutTable() {
        delay(100);
    }
}
