package io.github.maximgorbunov

import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import org.junit.jupiter.api.Test;
import kotlinx.coroutines.debug.DebugProbes
import org.junit.jupiter.api.BeforeAll

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
    fun plainFunctionTest() {
        plainFunction()
    }

    fun plainFunction() {
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

    companion object {
        @JvmStatic
        @BeforeAll
        fun setUp() {
            DebugProbes.install()
        }
      }
}
