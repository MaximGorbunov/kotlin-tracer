package io.github.maximgorbunov

import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import org.junit.jupiter.api.Test;
import kotlinx.coroutines.debug.DebugProbes
import org.junit.jupiter.api.BeforeAll

class InstrumentationTest {

    @Test
    fun simpleFunctionSwitchTable(): Unit = runBlocking {
            switchTableSuspend()
    }

    @Test
    fun simpleFunctionWithoutSwitchTable(): Unit = runBlocking {
    try {
        println("simpleFunctionWithoutSwitchTable")
        suspendWithoutTable()
        } catch(e: Throwable) { e.printStackTrace() }
    }

    suspend fun switchTableSuspend() {
    System.out.println("Here env:" + System.getenv("DYLD_INSERT_LIBRARIES"))
        delay(100)
        println("stub")
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
