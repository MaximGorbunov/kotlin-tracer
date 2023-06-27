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
configure_file(cpp/utils/config.h.in ${CMAKE_BINARY_DIR}/generated/config.h)

include_directories(${PROJECT_SOURCE_DIR}/src/main/cpp ${CMAKE_BINARY_DIR}/generated/)
include(GoogleTest)

add_executable(
        argsParser_test
        ${SOURCES_C} ${SOURCES_CPP}
        cpp/argsParserTest.cpp
)
target_link_libraries(
        argsParser_test
        agent
        GTest::gtest_main
)

add_executable(
        instrumentation_test
        ${SOURCES_C} ${SOURCES_CPP}
        cpp/instrumentationTest.cpp
)
target_link_libraries(
        instrumentation_test
        agent
        GTest::gtest_main
)


gtest_discover_tests(argsParser_test)
gtest_discover_tests(instrumentation_test)