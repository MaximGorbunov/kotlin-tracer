add_compile_options(
        "-O3" "-Werror" "-Wall" "-Wpedantic" "-Wextra" "-Wshadow" "-fexceptions" "-Wnon-virtual-dtor" "-Wno-unused-parameter"
        "$<$<CONFIG:DEBUG>:-O2;-DDEBUG=true;-fprofile-arcs;-ftest-coverage;-fno-omit-frame-pointer;-fsanitize=undefined;$<IF:$<BOOL:$ENV{THREAD_SANITIZER}>,-fsanitize=thread,-fsanitize=address>>"
)

add_link_options(
        "$<$<CONFIG:DEBUG>:-fprofile-arcs;-ftest-coverage;-fsanitize=undefined;$<IF:$<BOOL:$ENV{THREAD_SANITIZER}>,-fsanitize=thread,-fsanitize=address>>"
)

string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories($ENV{JDK_DIR}/include/${OS}/)
include_directories($ENV{JDK_DIR}/include/)

include_directories(cpp/)

file(GLOB_RECURSE SOURCES_CPP cpp/*.cpp)

set(NAME "agent")

add_library(${NAME} SHARED ${SOURCES_CPP})

set_property(GLOBAL PROPERTY AGENT_PATH "${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")