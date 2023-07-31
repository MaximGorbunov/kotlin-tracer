add_compile_options(
        "-O3" "-Werror" "-Wall" "-Wpedantic" "-Wextra" "-Wshadow" "-fexceptions" "-Wnon-virtual-dtor" "-Wno-unused-parameter" "-Wno-gnu"
        "$<$<CONFIG:DEBUG>:-O2;-DDEBUG=true;>"
        "$<$<BOOL:$ENV{THREAD_SANITIZER}>:-fsanitize=thread;-fsanitize=undefined;>"
        "$<$<BOOL:$ENV{ADDRESS_SANITIZER}>:-fsanitize=address;-fsanitize=undefined;>"
)

if (NOT APPLE)
    link_libraries("unwind")
endif(NOT APPLE)

link_directories(${PROJECT_SOURCE_DIR}/addr2Symbol/release/build/)
link_libraries(addr2Symbol)

add_link_options(
        "$<$<BOOL:$ENV{THREAD_SANITIZER}>:-fsanitize=thread;-fsanitize=undefined;>"
        "$<$<BOOL:$ENV{ADDRESS_SANITIZER}>:-fsanitize=address;-fsanitize=undefined;>"
)

string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories(${PROJECT_SOURCE_DIR}/addr2Symbol/src/main/)
include_directories($ENV{JAVA_HOME}/include/${OS}/)
include_directories($ENV{JAVA_HOME}/include/)

include_directories(cpp/)

file(GLOB_RECURSE SOURCES_CPP cpp/*.cpp)

set(NAME "agent")

add_library(${NAME} SHARED ${SOURCES_CPP})

set_property(GLOBAL PROPERTY AGENT_PATH "${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")