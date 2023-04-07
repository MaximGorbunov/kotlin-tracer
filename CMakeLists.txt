cmake_minimum_required(VERSION 3.10)
project(kotlin-tracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(
        "-Wall" "-Wpedantic" "-Wextra" "-Wshadow" "-fexceptions" "-Wnon-virtual-dtor" "-Wno-unused-parameter"
        "$<$<CONFIG:DEBUG>:-fsanitize=address;-DDEBUG>"
)

add_link_options(
        "$<$<CONFIG:DEBUG>:-fsanitize=address>"
)


string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories($ENV{JDK_DIR}/include/${OS}/)
include_directories($ENV{JDK_DIR}/include/)
include_directories(src/main/cpp)

file(GLOB_RECURSE SOURCES_CPP ${PROJECT_SOURCE_DIR}/src/main/cpp/*.cpp)

link_directories($ENV{JDK_DIR}/lib/server/)
link_libraries(jvm)

add_library(agent SHARED ${SOURCES_CPP})



include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
        hello_test
        ${SOURCES_C} ${SOURCES_CPP}
        src/test/cpp/argsParserTest.cpp
)
target_link_libraries(
        hello_test
        agent
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(hello_test)
