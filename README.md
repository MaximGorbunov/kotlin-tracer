# Kotlin tracer
## What is it for?
It's tracing agent that gathers information about method execution when trace exceeds threshold. It gathers: 
1. Icicle graph with mixed stacktrace, including both java and native frames.
2. Context switches count during trace with breakdown to voluntary switches and involuntary switches.
3. Garbage collector events during trace

## How to 

### Build
1. Make sure you have JAVA_HOME exported as environment variable
```bash

```

### Run
