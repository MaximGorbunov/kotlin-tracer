add_compile_options("-fno-omit-frame-pointer;-fsanitize=address;-fsanitize=undefined;-DDEBUG>")

add_link_options("-fsanitize=address;-fsanitize=undefined")

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/b796f7d44681514f58a683a3a71ff17c94edb0c1.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

get_property(AGENT_PATH GLOBAL PROPERTY AGENT_PATH)
set(CMAKE_TEST_RESOURCES_DIR "${PROJECT_SOURCE_DIR}/test/resources/")
set(GRADLEW_PATH "${PROJECT_SOURCE_DIR}/gradlew")

if (APPLE)
    set(ASAN_NAME "libclang_rt.asan_osx_dynamic.dylib")
elseif (UNIX)
    set(ASAN_NAME "libasan.so")
endif (APPLE)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${ASAN_NAME}
        OUTPUT_VARIABLE LIBASAN_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

file(GLOB_RECURSE MOCK_SOURCES_CPP cpp/mock/*.cpp)

string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories($ENV{JAVA_HOME}/include/${OS}/)
include_directories($ENV{JAVA_HOME}/include/)

configure_file(cpp/utils/config.h.in ${CMAKE_BINARY_DIR}/generated/config.h)

include_directories(${PROJECT_SOURCE_DIR}/src/main/cpp ${CMAKE_BINARY_DIR}/generated/)
include(GoogleTest)

macro(add_test test)
    string(REPLACE "/" "_" name ${test})
    add_executable(
            ${name}
            ${PROJECT_SOURCE_DIR}/src/test/cpp/${test}.cpp
    )
    target_link_libraries(
            ${name}
            agent
            GTest::gtest_main
    )
    gtest_discover_tests(${name})
endmacro()
macro(add_mock_test test mocks sources)
    string(REPLACE "/" "_" name ${test})
    add_executable(
            ${name}
            ${sources}
            ${PROJECT_SOURCE_DIR}/src/test/cpp/mock/${mocks}
            ${PROJECT_SOURCE_DIR}/src/test/cpp/${test}.cpp
    )
    target_link_libraries(
            ${name}
            GTest::gtest_main
            GTest::gmock_main
    )
    gtest_discover_tests(${name})
endmacro()

add_test(concurrentCollections/concurrentListTest)
add_test(concurrentCollections/concurrentMapTest)
add_test(argsParserTest)
add_test(instrumentationTest)
add_mock_test(jni/jniInstrumentatorTest agentThrow.cpp ${SOURCES_DIR}/jni/jni_instrumentator.cpp)