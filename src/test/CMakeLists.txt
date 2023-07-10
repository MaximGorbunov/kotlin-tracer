add_compile_options("-fprofile-arcs;-ftest-coverage;-fno-omit-frame-pointer;-fsanitize=address;-fsanitize=undefined;-DDEBUG>")

add_link_options("-fprofile-arcs;-ftest-coverage;-fsanitize=address;-fsanitize=undefined")

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
    message("LINUX HERE")
    set(ASAN_NAME "libasan.so")
endif (APPLE)
execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${ASAN_NAME}
        OUTPUT_VARIABLE LIBASAN_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

file(GLOB_RECURSE TEST_SOURCES_CPP cpp/*.cpp)

configure_file(cpp/utils/config.h.in ${CMAKE_BINARY_DIR}/generated/config.h)

include_directories(${PROJECT_SOURCE_DIR}/src/main/cpp ${CMAKE_BINARY_DIR}/generated/)
include(GoogleTest)

add_executable(
        all_tests
        ${SOURCES_C} ${SOURCES_CPP}
        ${TEST_SOURCES_CPP}
)

target_link_libraries(
        all_tests
        agent
        GTest::gtest_main
)

gtest_discover_tests(all_tests)