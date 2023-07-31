# Kotlin tracer
## What is it for?
It's tracing agent that gathers information about method execution when trace exceeds threshold. It gathers: 
1. Icicle graph with mixed stacktrace, including both java and native frames.
2. Context switches count during trace with breakdown to voluntary switches and involuntary switches.
3. Garbage collector events during trace
4. Kotlin's suspension points during trace and time elapsed inside them

## How to 

### Build
Make sure you have JAVA_HOME exported as environment variable
#### Initialize repository if you doing it first time:
```bash
git clone git@github.com:MaximGorbunov/kotlin-tracer.git
cd kotlin-tracer
git submodule update --init --remote kotlin-tracer-java
mkdir release && cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
```
#### Build java instrumentation jar
```bash
./gradlew :kotlin-tracer-java:jar
```
#### Build JVMTI agent
```bash
cmake --build release --target agent
```
#### Agents path:
Java instrumentation agent: kotlin-tracer-java/build/libs/kotlin-tracer-java.jar

JVMTI agent: release/agent/libagent.so for Linux or release/agent/libagent.dylib for MacOS

### Run
In order to use kotlin-tracer agent you have to set JVM options:
1. Add ```-Dkotlinx.coroutines.debug``` to enable kotlin coroutine debug
2. Add ```-agentpath:/path/to/libagent.so(dylib)=<replace it with options>```
#### Options
#### Sampling period in nanoseconds:
```text 
period=1000000
```
#### Full name of method for tracing. 
Syntax: package/name/ClassName.methodName
```text
method=io/test/Example.method
```
#### Threshold for plotting icicle graph of trace
```text
threshold=10
```
#### Java instrumentation agent
```text
jarPath=./kotlin-tracer/build/libs/kotlin-tracer.jar,
```
#### Icicle graph output path
outputPath=/tmp

#### Complete example
```text
-agentpath:./libagent.so=period=1000000,method=io/test/Example.method,threshold=10,jarPath=./kotlin-tracer/build/libs/kotlin-tracer.jar,outputPath=/tmp
```